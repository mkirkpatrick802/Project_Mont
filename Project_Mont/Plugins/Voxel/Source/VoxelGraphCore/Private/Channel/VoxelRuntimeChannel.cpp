// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Channel/VoxelRuntimeChannel.h"
#include "Channel/VoxelWorldChannel.h"
#include "Channel/VoxelChannelEvaluator.h"
#include "VoxelDependency.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelRuntimeChannel);

FVoxelFutureValue FVoxelRuntimeChannel::Get(const FVoxelQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FVoxelChannelEvaluator> ChannelEvaluator = MakeVoxelShared<FVoxelChannelEvaluator>(AsShared(), Query);
	const FVoxelFutureValue FutureValue = ChannelEvaluator->Compute();

	return
		MakeVoxelTask()
		.Dependency(FutureValue)
		.Execute(Definition.Type, [ChannelEvaluator, FutureValue]
		{
			// Keep ChannelEvaluator alive until we are done
			(void)ChannelEvaluator;
			return FutureValue;
		});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelBrushPriority FVoxelRuntimeChannel::GetFullPriority(
	const int32 Priority,
	const FString& GraphPath,
	const FString* NodeId,
	const FString& InstancePath)
{
	ensure(!GraphPath.IsEmpty());
	ensure(!InstancePath.IsEmpty());

	FVoxelBrushPriority Result;
	Result.BasePriority = FMath::Clamp<int64>(int64(Priority) + MAX_uint16 / 2, 0, MAX_uint16 - 1);
	Result.GraphHash = FVoxelUtilities::MurmurHash(FCrc::StrCrc32(*GraphPath));
	Result.InstanceHash = FVoxelUtilities::MurmurHash(FCrc::StrCrc32(*InstancePath));
	Result.NodeHash =
		NodeId
		? FVoxelUtilities::MurmurHash(FCrc::StrCrc32(**NodeId))
		// For preview meshes we want to skip any brush node in the current graph
		: 0;
	return Result;
}

FVoxelBrushPriority FVoxelRuntimeChannel::GetPriority(const int32 Priority)
{
	FVoxelBrushPriority Result;
	Result.BasePriority = FMath::Clamp<int64>(int64(Priority) + MAX_uint16 / 2, 0, MAX_uint16 - 1);
	Result.GraphHash = MAX_uint16;
	Result.InstanceHash = MAX_uint16;
	Result.NodeHash = MAX_uint16;
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimeChannel::FVoxelRuntimeChannel(
	const TSharedRef<FVoxelWorldChannel>& WorldChannel,
	const FVoxelTransformRef& RuntimeLocalToWorld)
	: WorldChannel(WorldChannel)
	, Definition(WorldChannel->Definition)
	, RuntimeLocalToWorld(RuntimeLocalToWorld)
	, Dependency(FVoxelDependency::Create(STATIC_FNAME("Channel"), WorldChannel->Definition.Name))
{
}

void FVoxelRuntimeChannel::AddBrush(
	const FVoxelBrushId BrushId,
	const TSharedRef<const FVoxelBrush>& Brush)
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelTransformRef BrushToRuntime = Brush->LocalToWorld * RuntimeLocalToWorld.Inverse();
	const TSharedRef<FRuntimeBrush> RuntimeBrush = MakeVoxelShared<FRuntimeBrush>(Brush, BrushToRuntime);

	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		RuntimeBrushes_RequiresLock.Add_EnsureNew(BrushId, RuntimeBrush);
	}

	BrushToRuntime.AddOnChanged(MakeWeakPtrDelegate(RuntimeBrush, MakeWeakPtrLambda(this, [this, &RuntimeBrush = *RuntimeBrush](const FMatrix& NewTransform)
	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		if (RuntimeBrush.RuntimeBounds_RequiresLock.IsSet())
		{
			FVoxelDependency::FInvalidationParameters Parameters;
			Parameters.Bounds = RuntimeBrush.RuntimeBounds_RequiresLock.GetValue();
			Parameters.LessOrEqualTag = RuntimeBrush.Priority.Raw;
			Dependency->Invalidate(Parameters);
		}

		RuntimeBrush.RuntimeBounds_RequiresLock = RuntimeBrush.Brush->LocalBounds.TransformBy(NewTransform);

		if (RuntimeBrush.Brush->LocalBounds.IsInfinite())
		{
			RuntimeBrush.RuntimeBounds_RequiresLock = FVoxelBox::Infinite;
		}

		FVoxelDependency::FInvalidationParameters Parameters;
		Parameters.Bounds = RuntimeBrush.RuntimeBounds_RequiresLock.GetValue();
		Parameters.LessOrEqualTag = RuntimeBrush.Priority.Raw;
		Dependency->Invalidate(Parameters);
	})));
}

void FVoxelRuntimeChannel::RemoveBrush(
	const FVoxelBrushId BrushId,
	const TSharedRef<const FVoxelBrush>& Brush)
{
	VOXEL_FUNCTION_COUNTER();

	TSharedPtr<FRuntimeBrush> RuntimeBrush;
	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		if (!ensure(RuntimeBrushes_RequiresLock.RemoveAndCopyValue(BrushId, RuntimeBrush)))
		{
			return;
		}
	}

	FVoxelDependency::FInvalidationParameters Parameters;
	Parameters.Bounds = RuntimeBrush->RuntimeBounds_RequiresLock.GetValue();
	Parameters.LessOrEqualTag = Brush->Priority.Raw;
	Dependency->Invalidate(Parameters);
}