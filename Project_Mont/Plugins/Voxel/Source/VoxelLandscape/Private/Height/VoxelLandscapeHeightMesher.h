// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeState.h"
#include "VoxelLandscapeChunkKey.h"

class FVoxelLandscapeHeightMesh;

class FVoxelLandscapeHeightMesher : public TSharedFromThis<FVoxelLandscapeHeightMesher>
{
public:
	const FVoxelLandscapeChunkKey ChunkKey;
	const TSharedRef<FVoxelLandscapeState> State;
	const int32 DataSize;
	const int32 DetailDataSize;

	explicit FVoxelLandscapeHeightMesher(
		const FVoxelLandscapeChunkKey& ChunkKey,
		const TSharedRef<FVoxelLandscapeState>& State)
		: ChunkKey(ChunkKey)
		, State(State)
		, DataSize(State->ChunkSize + 1)
		, DetailDataSize(State->ChunkSize * State->DetailTextureSize + 2)
	{
	}

	TVoxelFuture<FVoxelLandscapeHeightMesh> Build();

private:
	float MinHeight = 0;
	float MaxHeight = 0;

	TVoxelArray<float> Heights;
	TVoxelArray<float> DetailHeights;
	TVoxelArray<FVoxelOctahedron> Normals;

	void FinalizeHeights();
	void ComputeNormals();
};