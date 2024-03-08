// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"

DECLARE_VOXEL_SPARSE_INDEX(FVoxelBrushId);

struct VOXELGRAPHCORE_API FVoxelBrushPriority
{
	union
	{
		struct
		{
			uint64 NodeHash : 16;
			// Actor/component hash, might not be deterministic
			uint64 InstanceHash : 16;
			uint64 GraphHash : 16;
			uint64 BasePriority : 16;
		};
		uint64 Raw = 0;
	};

	explicit FVoxelBrushPriority() = default;

	static FVoxelBrushPriority Max()
	{
		FVoxelBrushPriority Result;
		Result.Raw = MAX_uint64;
		return Result;
	}

	FORCEINLINE bool operator<(const FVoxelBrushPriority& Other) const
	{
		return Raw < Other.Raw;
	}
	FORCEINLINE bool operator<=(const FVoxelBrushPriority& Other) const
	{
		return Raw <= Other.Raw;
	}

	FORCEINLINE bool operator>(const FVoxelBrushPriority& Other) const
	{
		return Raw > Other.Raw;
	}
	FORCEINLINE bool operator>=(const FVoxelBrushPriority& Other) const
	{
		return Raw >= Other.Raw;
	}

	FORCEINLINE bool operator==(const FVoxelBrushPriority& Other) const
	{
		return Raw == Other.Raw;
	}
	FORCEINLINE bool operator!=(const FVoxelBrushPriority& Other) const
	{
		return Raw != Other.Raw;
	}
};
checkStatic(sizeof(FVoxelBrushPriority) == sizeof(uint64));

class VOXELGRAPHCORE_API FVoxelBrush
{
public:
	const FName DebugName;
	const FName DebugNameWithBrushPrefix;
	const FVoxelBrushPriority Priority;
	const FVoxelBox LocalBounds;
	const FVoxelTransformRef LocalToWorld;
	const FVoxelComputeValue Compute;

	VOXEL_COUNT_INSTANCES();

	FVoxelBrush(
		const FName DebugName,
		const FVoxelBrushPriority Priority,
		const FVoxelBox& LocalBounds,
		const FVoxelTransformRef& LocalToWorld,
		FVoxelComputeValue&& Compute)
		: DebugName(DebugName)
		, DebugNameWithBrushPrefix(TEXTVIEW("Brush: ") + DebugName)
		, Priority(Priority)
		, LocalBounds(LocalBounds)
		, LocalToWorld(LocalToWorld)
		, Compute(MoveTemp(Compute))
	{
	}
};