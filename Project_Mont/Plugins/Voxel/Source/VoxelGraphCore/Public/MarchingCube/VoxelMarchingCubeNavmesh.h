// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelMarchingCubeNavmeshMemory, "Voxel Marching Cube Navmesh Memory");

class VOXELGRAPHCORE_API FVoxelMarchingCubeNavmesh
{
public:
	const FVoxelBox LocalBounds;
	const TVoxelArray<int32> Indices;
	const TVoxelArray<FVector3f> Vertices;

	FVoxelMarchingCubeNavmesh(
		const FVoxelBox& LocalBounds,
		TVoxelArray<int32>&& Indices,
		TVoxelArray<FVector3f>&& Vertices)
		: LocalBounds(LocalBounds)
		, Indices(MoveTemp(Indices))
		, Vertices(MoveTemp(Vertices))
	{
	}

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelMarchingCubeNavmeshMemory);

	int64 GetAllocatedSize() const
	{
		return Indices.GetAllocatedSize() + Vertices.GetAllocatedSize();
	}
};