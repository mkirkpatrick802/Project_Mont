// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeChunkKey.h"

class FVoxelLandscapeState;

struct FVoxelLandscapeChunkTree
{
public:
	const FVoxelLandscapeState& State;
	const FVector CameraChunkPosition;
	const double TargetChunkSizeOnScreenSquared;

	TVoxelArray<FVoxelLandscapeChunkKeyWithTransition> ChunkKeys;

	explicit FVoxelLandscapeChunkTree(const FVoxelLandscapeState& State);

	void Build();

private:
	void Traverse(FVoxelLandscapeChunkKey ChunkKey);
	bool ShouldSubdivide(FVoxelLandscapeChunkKey ChunkKey) const;
};