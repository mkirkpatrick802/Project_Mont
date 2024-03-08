// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class FVoxelDependency;
class FVoxelDependencyTracker;

DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelDependencies, "Dependencies");

class VOXELGRAPHCORE_API FVoxelDependencyInvalidationScope
{
public:
	FVoxelDependencyInvalidationScope();
	~FVoxelDependencyInvalidationScope();

private:
	TVoxelChunkedArray<TWeakPtr<FVoxelDependencyTracker>> Trackers;

	void Invalidate();

	friend FVoxelDependency;
};

class VOXELGRAPHCORE_API FVoxelDependency : public TSharedFromThis<FVoxelDependency>
{
public:
	const FName ClassName;
	const FName InstanceName;

	static TSharedRef<FVoxelDependency> Create(
		const FName ClassName,
		const FName InstanceName)
	{
		return TSharedRef<FVoxelDependency>(new FVoxelDependency(ClassName, InstanceName));
	}
	~FVoxelDependency() = default;
	UE_NONCOPYABLE(FVoxelDependency);

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelDependencies);

	int64 GetAllocatedSize() const
	{
		return TrackerRefs_RequiresLock.GetAllocatedSize();
	}

	struct FInvalidationParameters
	{
		TOptional<FVoxelBox> Bounds;
		TOptional<uint64> LessOrEqualTag;
	};
	void Invalidate(FInvalidationParameters Parameters = {});

private:
	FVoxelCriticalSection CriticalSection;

	struct FTrackerRef
	{
		TWeakPtr<FVoxelDependencyTracker> WeakTracker;

		bool bHasBounds = false;
		FVoxelBox Bounds;

		bool bHasTag = false;
		uint64 Tag = 0;

		FORCEINLINE bool operator==(const FTrackerRef& Other) const
		{
			if (WeakTracker != Other.WeakTracker ||
				bHasBounds != Other.bHasBounds ||
				bHasTag != Other.bHasTag)
			{
				return false;
			}

			if (bHasBounds &&
				Bounds != Other.Bounds)
			{
				return false;
			}

			if (bHasTag &&
				Tag != Other.Tag)
			{
				return false;
			}

			return true;
		}
	};
	TVoxelChunkedSparseArray<FTrackerRef> TrackerRefs_RequiresLock;

	FVoxelDependency(
		FName ClassName,
		FName InstanceName);

	friend FVoxelDependencyTracker;
};