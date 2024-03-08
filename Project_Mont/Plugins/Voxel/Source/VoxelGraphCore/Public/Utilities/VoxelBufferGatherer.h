// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelBaseBuffers.h"

struct VOXELGRAPHCORE_API FVoxelBufferGatherer
{
public:
	static TSharedRef<const FVoxelBuffer> Gather(
		const FVoxelBuffer& Buffer,
		const FVoxelInt32Buffer& Indices);

	template<typename T>
	static TSharedRef<const T> Gather(
		const T& Buffer,
		const FVoxelInt32Buffer& Indices)
	{
		return CastChecked<T>(Gather(static_cast<const FVoxelBuffer&>(Buffer), Indices));
	}

private:
	static void Gather(
		FVoxelTerminalBuffer& OutBuffer,
		const FVoxelTerminalBuffer& Buffer,
		const FVoxelInt32Buffer& Indices);
};