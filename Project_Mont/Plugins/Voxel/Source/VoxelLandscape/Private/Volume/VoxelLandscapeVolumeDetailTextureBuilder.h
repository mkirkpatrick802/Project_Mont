// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeChunkKey.h"

class FVoxelLandscapeVolumeMeshData;

class FVoxelLandscapeVolumeDetailTextureBuilder
{
public:
	const FVoxelLandscapeChunkKey ChunkKey;
	const TSharedRef<FVoxelLandscapeState> State;
	const TSharedRef<FVoxelLandscapeVolumeMeshData> MeshData;

	FVoxelLandscapeVolumeDetailTextureBuilder(
		const TSharedRef<FVoxelLandscapeState>& State,
		const TSharedRef<FVoxelLandscapeVolumeMeshData>& MeshData);

	FVoxelFuture Build();
};