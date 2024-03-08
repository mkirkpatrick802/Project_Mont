// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelPointChunkRef.h"
#include "Point/VoxelPointSubsystem.h"
#include "VoxelQuery.h"
#include "VoxelRuntime.h"
#include "VoxelTaskHelpers.h"
#include "VoxelRuntimeProvider.h"
#include "VoxelDependencyTracker.h"
#include "VoxelTerminalGraphInstance.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, float, GVoxelPointCacheDuration, 1,
	"voxel.point.CacheDuration",
	"Duration, in seconds, for which to cache points");

class FVoxelPointChunkCache : public FVoxelSingleton
{
public:
	//~ Begin FVoxelSingleton Interface
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();
		VOXEL_SCOPE_LOCK(CriticalSection);

		const double KillTime = FPlatformTime::Seconds() - GVoxelPointCacheDuration;
		for (auto It = ChunkRefToData_RequiresLock.CreateIterator(); It; ++It)
		{
			if (It.Value()->LastAccessTime < KillTime ||
				It.Value()->DependencyTracker->IsInvalidated())
			{
				It.RemoveCurrent();
			}
		}
	}
	//~ End FVoxelSingleton Interface

	TSharedPtr<const FVoxelPointSet> GetPoints(
		const FVoxelPointChunkRef& ChunkRef,
		FString* OutError)
	{
		VOXEL_FUNCTION_COUNTER();
		VOXEL_SCOPE_LOCK(CriticalSection);
		VOXEL_SCOPE_COUNTER_FORMAT("%s", *ChunkRef.ChunkProviderRef.ToDebugString());

		if (const TSharedPtr<const FData> Data = ChunkRefToData_RequiresLock.FindRef(ChunkRef))
		{
			if (!Data->DependencyTracker->IsInvalidated())
			{
				Data->LastAccessTime = FPlatformTime::Seconds();
				if (OutError)
				{
					*OutError = Data->Error;
				}
				return Data->PointSet;
			}
		}
		ensure(IsInGameThread());

		UWorld* World = ChunkRef.GetWorld();
		if (!World)
		{
			if (OutError)
			{
				*OutError = "Invalid World";
			}
			return nullptr;
		}

		const TSharedRef<FVoxelRuntimeInfo> RuntimeInfo =
			FVoxelRuntimeInfoBase::MakeFromWorld(World)
			.EnableParallelTasks()
			.MakeRuntimeInfo_RequiresDestroy();
		ON_SCOPE_EXIT
		{
			RuntimeInfo->Destroy();
		};

		const TSharedRef<FData> Data = MakeVoxelShared<FData>();
		ChunkRefToData_RequiresLock.Add_CheckNew(ChunkRef, Data);

		const TSharedPtr<const FVoxelChunkedPointSet> ChunkedPointSet = ChunkRef.ChunkProviderRef.GetChunkedPointSet(&Data->Error);
		if (!ChunkedPointSet ||
			!ensure(ChunkedPointSet->GetChunkSize() == ChunkRef.ChunkSize))
		{
			return nullptr;
		}

		Data->DependencyTracker = FVoxelDependencyTracker::Create(STATIC_FNAME("FVoxelPointHandle"));

		bool bSuccess = true;
		TOptional<FVoxelPointSet> OptionalPoints = FVoxelTaskHelpers::TryRunSynchronously(
			FVoxelTerminalGraphInstance::MakeDummy(RuntimeInfo),
			[&]() -> TVoxelFutureValue<FVoxelPointSet>
			{
				const TVoxelFutureValue<FVoxelPointSet> FuturePoints = ChunkedPointSet->GetPoints(*Data->DependencyTracker, ChunkRef.ChunkMin);
				if (!FuturePoints.IsValid())
				{
					bSuccess = false;
					return FVoxelPointSet();
				}

				return FuturePoints;
			},
			&Data->Error);

		if (!bSuccess ||
			!OptionalPoints.IsSet())
		{
			return nullptr;
		}

		Data->PointSet = MakeSharedCopy(MoveTemp(*OptionalPoints));
		return Data->PointSet;
	}

private:
	struct FData
	{
		mutable double LastAccessTime = FPlatformTime::Seconds();
		FString Error;
		TSharedPtr<const FVoxelPointSet> PointSet;
		TSharedPtr<FVoxelDependencyTracker> DependencyTracker;
	};

	FVoxelCriticalSection CriticalSection;
	TVoxelMap<FVoxelPointChunkRef, TSharedPtr<const FData>> ChunkRefToData_RequiresLock;
};

FVoxelPointChunkCache* GVoxelPointChunkCache = new FVoxelPointChunkCache();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelPointChunkProviderRef::ToDebugString() const
{
	VOXEL_FUNCTION_COUNTER();

	TStringBuilderWithBuffer<TCHAR, NAME_SIZE> Result;

	for (const FVoxelGraphNodeRef& CallstackNodeRef : TerminalGraphInstanceCallstack)
	{
		Result += TEXT("/");
		CallstackNodeRef.AppendString(Result);
	}

	Result += TEXT("/");
	NodeRef.AppendString(Result);

	return FString(Result.ToView());
}

TSharedPtr<FVoxelRuntime> FVoxelPointChunkProviderRef::GetRuntime(FString* OutError) const
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!RuntimeProvider.IsValid())
	{
		if (OutError)
		{
			*OutError = "Invalid RuntimeProvider";
		}
		return {};
	}

	const TSharedPtr<FVoxelRuntime> Runtime = RuntimeProvider->GetRuntime();
	if (!Runtime)
	{
		if (OutError)
		{
			*OutError = "Runtime is destroyed";
		}
		return {};
	}

	return Runtime;
}

TSharedPtr<const FVoxelChunkedPointSet> FVoxelPointChunkProviderRef::GetChunkedPointSet(FString* OutError) const
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime(OutError);
	if (!Runtime)
	{
		if (OutError)
		{
			*OutError = "Invalid Runtime";
		}
		return nullptr;
	}

	const TSharedPtr<FVoxelPointSubsystem> Subsystem = Runtime->FindSubsystem<FVoxelPointSubsystem>();
	if (!ensure(Subsystem))
	{
		return nullptr;
	}

	TSharedPtr<const FVoxelChunkedPointSet> ChunkedPointSet;
	{
		VOXEL_SCOPE_LOCK(Subsystem->CriticalSection);
		ChunkedPointSet = Subsystem->ChunkProviderToChunkedPointSet_RequiresLock.FindRef(*this);
	}

	if (!ChunkedPointSet)
	{
		if (OutError)
		{
			*OutError = "Could not find matching ChunkedPointSet";
		}
		return nullptr;
	}

	return ChunkedPointSet;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<const FVoxelPointSet> FVoxelPointChunkRef::GetPoints(FString* OutError) const
{
	return GVoxelPointChunkCache->GetPoints(*this, OutError);
}

bool FVoxelPointChunkRef::NetSerialize(FArchive& Ar, UPackageMap& Map)
{
	VOXEL_FUNCTION_COUNTER();

	if (Ar.IsSaving())
	{
		ensure(IsValid());

		UObject* RuntimeProvider = ChunkProviderRef.RuntimeProvider.GetObject();
		ensure(RuntimeProvider);

		if (!ensure(Map.SerializeObject(Ar, UObject::StaticClass(), RuntimeProvider)))
		{
			return false;
		}

		{
			int32 Num = ChunkProviderRef.TerminalGraphInstanceCallstack.Num();
			Ar << Num;
		}

		for (FVoxelGraphNodeRef& NodeRef : ChunkProviderRef.TerminalGraphInstanceCallstack)
		{
			if (!ensure(NodeRef.NetSerialize(Ar, Map)))
			{
				return false;
			}
		}

		if (!ensure(ChunkProviderRef.NodeRef.NetSerialize(Ar, Map)))
		{
			return false;
		}

		Ar << ChunkMin;
		Ar << ChunkSize;
		return true;
	}
	else if (Ar.IsLoading())
	{
		UObject* RuntimeProvider = nullptr;
		if (!ensure(Map.SerializeObject(Ar, UObject::StaticClass(), RuntimeProvider)) ||
			!ensure(RuntimeProvider) ||
			!ensure(RuntimeProvider->Implements<UVoxelRuntimeProvider>()))
		{
			return false;
		}

		ChunkProviderRef.RuntimeProvider = CastChecked<IVoxelRuntimeProvider>(RuntimeProvider);

		{
			int32 Num = 0;
			Ar << Num;

			ChunkProviderRef.TerminalGraphInstanceCallstack.Reset();
			ChunkProviderRef.TerminalGraphInstanceCallstack.SetNum(Num);
		}

		for (FVoxelGraphNodeRef& NodeRef : ChunkProviderRef.TerminalGraphInstanceCallstack)
		{
			if (!ensure(NodeRef.NetSerialize(Ar, Map)))
			{
				return false;
			}
		}

		if (!ensure(ChunkProviderRef.NodeRef.NetSerialize(Ar, Map)))
		{
			return false;
		}

		Ar << ChunkMin;
		Ar << ChunkSize;
		return true;
	}
	else
	{
		ensure(false);
		return false;
	}
}

FArchive& operator<<(FArchive& Ar, FVoxelPointChunkProviderRef& Ref)
{
	UObject* Object = Ref.RuntimeProvider.GetObject();
	Ar << Object;

	if (Ar.IsLoading())
	{
		Ref.RuntimeProvider = CastEnsured<IVoxelRuntimeProvider>(Object);
	}

	Ar << Ref.TerminalGraphInstanceCallstack;
	Ar << Ref.NodeRef;
	return Ar;
}

uint32 GetTypeHash(const FVoxelPointChunkProviderRef& Ref)
{
	checkStatic(sizeof(FMinimalName) == sizeof(uint64));

	TVoxelInlineArray<FMinimalName, 16> NodeIds;
	for (const FVoxelGraphNodeRef& NodeRef : Ref.TerminalGraphInstanceCallstack)
	{
		NodeIds.Add(FMinimalName(NodeRef.NodeId));
	}

	return FVoxelUtilities::MurmurHashMulti(
		GetTypeHash(Ref.RuntimeProvider),
		FVoxelUtilities::MurmurHashBytes(MakeByteVoxelArrayView(NodeIds), NodeIds.Num()),
		GetTypeHash(Ref.NodeRef));
}