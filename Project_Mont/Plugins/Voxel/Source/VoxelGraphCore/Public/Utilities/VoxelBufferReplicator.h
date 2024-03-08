// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelBaseBuffers.h"

struct VOXELGRAPHCORE_API FVoxelBufferReplicator
{
public:
	static TSharedRef<const FVoxelBuffer> Replicate(
		const FVoxelBuffer& Buffer,
		TConstVoxelArrayView<int32> Counts,
		int32 NewNum);

private:
	static void Replicate(
		FVoxelTerminalBuffer& OutBuffer,
		const FVoxelTerminalBuffer& Buffer,
		TConstVoxelArrayView<int32> Counts,
		int32 NewNum);
};