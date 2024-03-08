// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscapeChunkKey.h"
#include "VoxelLandscapeState.h"

FVoxelBox2D FVoxelLandscapeChunkKey::GetQueriedBounds2D(const FVoxelLandscapeState& State) const
{
	return FVoxelBox2D(GetQueriedBounds(State));
}

FVoxelBox FVoxelLandscapeChunkKey::GetQueriedBounds(const FVoxelLandscapeState& State) const
{
	const int32 Step = 1 << LOD;

	FVoxelIntBox VoxelBounds =
		GetChunkKeyBounds()
		.Scale(State.ChunkSize);

	// Padding
	VoxelBounds.Max += FIntVector(Step);
	// Normals
	VoxelBounds = VoxelBounds.Extend(Step);

	return VoxelBounds.ToVoxelBox();
}