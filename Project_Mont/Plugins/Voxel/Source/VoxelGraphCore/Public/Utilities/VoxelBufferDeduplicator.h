// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelBaseBuffers.h"

struct VOXELGRAPHCORE_API FVoxelBufferDeduplicator
{
public:
	static void Deduplicate(
		const FVoxelTerminalBuffer& Buffer,
		FVoxelInt32Buffer& OutIndices,
		FVoxelTerminalBuffer& OutPalette);
};