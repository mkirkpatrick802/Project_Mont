// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeState.h"
#include "VoxelLandscapeChunkKey.h"

class FVoxelLandscapeNaniteMesh;

class FVoxelLandscapeNaniteMesher : public TSharedFromThis<FVoxelLandscapeNaniteMesher>
{
public:
	const FVoxelLandscapeChunkKey ChunkKey;
	const TSharedRef<FVoxelLandscapeState> State;
	const int32 HeightsSize;

	explicit FVoxelLandscapeNaniteMesher(
		const FVoxelLandscapeChunkKey& ChunkKey,
		const TSharedRef<FVoxelLandscapeState>& State)
		: ChunkKey(ChunkKey)
		, State(State)
		, HeightsSize(State->ChunkSize + 3)
	{
	}

	TVoxelFuture<FVoxelLandscapeNaniteMesh> Build();

private:
	float MinHeight = 0;
	float MaxHeight = 0;
	TVoxelArray<float> Heights;

	TSharedRef<FVoxelLandscapeNaniteMesh> Finalize();
};