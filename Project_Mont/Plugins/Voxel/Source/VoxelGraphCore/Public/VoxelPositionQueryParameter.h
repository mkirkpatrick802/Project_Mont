// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelPositionQueryParameter.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelPositionQueryParameter : public FVoxelQueryParameter
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_PARAMETER_BODY()

public:
	bool IsGradient() const
	{
		return bIsGradient;
	}
	bool IsGrid() const
	{
		return Grid.IsValid();
	}

	struct FGrid
	{
		FVector3f Start;
		float Step = 0.f;
		FIntVector Size;
	};
	const FGrid& GetGrid() const
	{
		return *Grid;
	}

	FVoxelBox GetBounds() const;
	FVoxelVectorBuffer GetPositions() const;

public:
	bool TryFilter(
		const FVoxelBox& BoundsToFilter,
		FVoxelInt32Buffer& OutIndices,
		FVoxelVectorBuffer& OutPositions) const;

public:
	void Initialize(
		const FVoxelVectorBuffer& NewPositions,
		const TOptional<FVoxelBox>& NewBounds = {});
	void Initialize(
		TVoxelUniqueFunction<FVoxelVectorBuffer()>&& NewCompute,
		const TOptional<FVoxelBox>& NewBounds = {});
	void InitializeGradient(
		const FVoxelVectorBuffer& NewPositions,
		const TOptional<FVoxelBox>& NewBounds = {});

	void InitializeGrid(
		const FVector3f& Start,
		float Step,
		const FIntVector& Size);

private:
	struct FCell
	{
		int32 Num = 0;
		int32 Index = 0;
	};
	struct FCells
	{
		FVoxelInt64Buffer Cells;
		FVoxelInt32Buffer Indices;
		int32 Step = 0;
		FVector Offset = FVector(ForceInit);
		FIntVector GridSize = FIntVector(ForceInit);
	};

	TSharedPtr<const FGrid> Grid;
	bool bIsGradient = false;
	TOptional<FVoxelBox> PrecomputedBounds;
	TSharedPtr<const TVoxelUniqueFunction<FVoxelVectorBuffer()>> Compute;

	mutable FVoxelCriticalSection CriticalSection;
	mutable TOptional<FVoxelBox> CachedBounds_RequiresLock;
	mutable TOptional<FVoxelVectorBuffer> CachedPositions_RequiresLock;
	mutable TOptional<FCells> CachedCells_RequiresLock;

	void CheckBounds() const;
	FVoxelBox GetBounds_RequiresLock() const;
	FVoxelVectorBuffer GetPositions_RequiresLock() const;
	const FCells& GetCells_RequiresLock() const;

	friend class FVoxelPositionQueryParameterHelper;
};