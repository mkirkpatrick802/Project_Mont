// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Channel/VoxelWorldChannel.h"
#include "Channel/VoxelChannelManager.h"
#include "Channel/VoxelRuntimeChannel.h"
#include "VoxelDependency.h"
#include "Engine/Engine.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelBrush);

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelShowBrushBounds, false,
	"voxel.ShowBrushBounds",
	"");

FVoxelBrushRef::~FVoxelBrushRef()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<FVoxelWorldChannel> Channel = WeakChannel.Pin();
	if (!Channel)
	{
		return;
	}

	// Don't invalidate while CriticalSection is locked
	const FVoxelDependencyInvalidationScope DependencyInvalidationScope;

	VOXEL_SCOPE_LOCK(Channel->CriticalSection);
	Channel->RemoveBrush_RequiresLock(BrushId);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelWorldChannel::AddBrush(
	const TSharedRef<const FVoxelBrush>& Brush,
	TSharedPtr<FVoxelBrushRef>& OutBrushRef)
{
	VOXEL_FUNCTION_COUNTER();

	// Don't invalidate while CriticalSection is locked
	const FVoxelDependencyInvalidationScope DependencyInvalidationScope;

	// Never delete brush refs inside CriticalSection
	const TSharedPtr<FVoxelBrushRef> BrushRefToDelete = MoveTemp(OutBrushRef);
	ensure(!OutBrushRef);

	VOXEL_SCOPE_LOCK(CriticalSection);

	if (BrushRefToDelete &&
		BrushRefToDelete.IsUnique() &&
		BrushRefToDelete->WeakChannel == AsWeak())
	{
		// Remove any previous brush before adding it again to avoid priority duplicates
		RemoveBrush_RequiresLock(BrushRefToDelete->BrushId);
		BrushRefToDelete->WeakChannel = {};
		BrushRefToDelete->BrushId = {};
	}

#if VOXEL_DEBUG && 0
	for (const TSharedPtr<const FVoxelBrush>& OtherBrush : Brushes_RequiresLock)
	{
		ensure(Brush->Priority != OtherBrush->Priority);
	}
#endif

	const FVoxelBrushId BrushId = Brushes_RequiresLock.Add(Brush);
	OutBrushRef = MakeVoxelShareable(new (GVoxelMemory) FVoxelBrushRef(AsShared(), BrushId));

	for (int32 Index = 0; Index < WeakRuntimeChannels_RequiresLock.Num(); Index++)
	{
		const TSharedPtr<FVoxelRuntimeChannel> RuntimeChannel = WeakRuntimeChannels_RequiresLock[Index].Pin();
		if (!RuntimeChannel)
		{
			WeakRuntimeChannels_RequiresLock.RemoveAtSwap(Index);
			Index--;
			continue;
		}

		RuntimeChannel->AddBrush(BrushId, Brush);
	}
}

TSharedRef<FVoxelRuntimeChannel> FVoxelWorldChannel::GetRuntimeChannel(
	const FVoxelTransformRef& RuntimeLocalToWorld,
	FVoxelRuntimeChannelCache& Cache)
{
	VOXEL_SCOPE_LOCK(Cache.CriticalSection);

	if (const TSharedPtr<FVoxelRuntimeChannel>* ChannelPtr = Cache.Channels_RequiresLock.Find(Definition.Name))
	{
		checkVoxelSlow((**ChannelPtr).RuntimeLocalToWorld == RuntimeLocalToWorld);
		return ChannelPtr->ToSharedRef();
	}

	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FVoxelRuntimeChannel> Channel = MakeVoxelShareable(new(GVoxelMemory) FVoxelRuntimeChannel(AsShared(), RuntimeLocalToWorld));
	Cache.Channels_RequiresLock.Add_CheckNew(Definition.Name, Channel);

	VOXEL_SCOPE_LOCK(CriticalSection);

	WeakRuntimeChannels_RequiresLock.Add(Channel);

	for (int32 Index = 0; Index < Brushes_RequiresLock.GetMaxIndex(); Index++)
	{
		if (!Brushes_RequiresLock.IsAllocated(Index))
		{
			continue;
		}

		const FVoxelBrushId BrushId = Brushes_RequiresLock.MakeIndex(Index);
		Channel->AddBrush(BrushId, Brushes_RequiresLock[BrushId].ToSharedRef());
	}

	return Channel;
}

void FVoxelWorldChannel::DrawBrushBounds(
	const FObjectKey World,
	const FLinearColor& Color) const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	for (const TSharedPtr<const FVoxelBrush>& Brush : Brushes_RequiresLock)
	{
		FVoxelDebugDrawer(World)
		.OneFrame()
		.Color(Color)
		.DrawBox(Brush->LocalBounds, Brush->LocalToWorld.Get_NoDependency());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelWorldChannel::RemoveBrush_RequiresLock(const FVoxelBrushId BrushId)
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(CriticalSection.IsLocked());

	const TSharedRef<const FVoxelBrush> Brush = Brushes_RequiresLock[BrushId].ToSharedRef();
	Brushes_RequiresLock.RemoveAt(BrushId);

	for (int32 Index = 0; Index < WeakRuntimeChannels_RequiresLock.Num(); Index++)
	{
		const TSharedPtr<FVoxelRuntimeChannel> RuntimeChannel = WeakRuntimeChannels_RequiresLock[Index].Pin();
		if (!RuntimeChannel)
		{
			WeakRuntimeChannels_RequiresLock.RemoveAtSwap(Index);
			Index--;
			continue;
		}

		RuntimeChannel->RemoveBrush(BrushId, Brush);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelWorldChannelManager::RegisterChannel(const FVoxelChannelDefinition& ChannelDefinition)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	if (GVoxelChannelManager->FindChannelDefinition(ChannelDefinition.Name))
	{
		return false;
	}

	if (Channels_RequiresLock.Contains(ChannelDefinition.Name))
	{
		return false;
	}

	const TSharedRef<FVoxelWorldChannel> Channel = MakeVoxelShareable(new (GVoxelMemory) FVoxelWorldChannel(ChannelDefinition));
	Channels_RequiresLock.Add_CheckNew(ChannelDefinition.Name, Channel);
	return true;
}

TSharedPtr<FVoxelWorldChannel> FVoxelWorldChannelManager::FindChannel(const FName Name)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	if (const TSharedPtr<FVoxelWorldChannel>* ChannelPtr = Channels_RequiresLock.Find(Name))
	{
		return ChannelPtr->ToSharedRef();
	}

	const TOptional<FVoxelChannelDefinition> ChannelDefinition = GVoxelChannelManager->FindChannelDefinition(Name);
	if (!ChannelDefinition)
	{
		// Invalid channel
		return nullptr;
	}
	check(ChannelDefinition->Name == Name);

	const TSharedRef<FVoxelWorldChannel> Channel = MakeVoxelShareable(new (GVoxelMemory) FVoxelWorldChannel(*ChannelDefinition));
	Channels_RequiresLock.Add_CheckNew(Name, Channel);
	return Channel;
}

TVoxelArray<FName> FVoxelWorldChannelManager::GetValidChannelNames() const
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	TVoxelSet<FName> Result;
	Result.Append(Channels_RequiresLock.KeyArray());
	Result.Append(GVoxelChannelManager->GetValidChannelNames());
	return Result.Array();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelWorldChannelManager::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	if (!GVoxelShowBrushBounds)
	{
		return;
	}

	VOXEL_SCOPE_LOCK(CriticalSection);

	int32 Index = 0;
	for (const auto& It : Channels_RequiresLock)
	{
		FLinearColor Color;
		if (GEngine->LODColorationColors.IsValidIndex(Index))
		{
			Color = GEngine->LODColorationColors[Index];
		}
		else
		{
			Color = FColor(FVoxelUtilities::MurmurHash(Index));
		}
		Color.A = 1.f;

		Index++;

		// Avoid printing twice in PIE
		if (GetWorld()->GetNetMode() != NM_DedicatedServer)
		{
			GEngine->AddOnScreenDebugMessage(
				FVoxelUtilities::MurmurHash(this) ^ FVoxelUtilities::MurmurHash(It.Key),
				10 * FApp::GetDeltaTime(),
				Color.ToFColor(true),
				"Channel " + It.Key.ToString());
		}

		It.Value->DrawBrushBounds(GetWorld_AnyThread(), Color);
	}
}

TSharedPtr<FVoxelRuntimeChannel> FVoxelWorldChannelManager::FindRuntimeChannel(
	const UWorld* World,
	const FName ChannelName,
	const FVoxelTransformRef& Transform,
	const TSharedRef<FVoxelRuntimeChannelCache>& Cache)
{
	if (!ensure(World))
	{
		return nullptr;
	}

	const TSharedRef<FVoxelWorldChannelManager> ChannelManager = Get(World);
	const TSharedPtr<FVoxelWorldChannel> WorldChannel = ChannelManager->FindChannel(ChannelName);
	if (!WorldChannel)
	{
		return nullptr;
	}

	return WorldChannel->GetRuntimeChannel(Transform, *Cache);
}