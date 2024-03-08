// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelBaseBuffers.h"

struct VOXELGRAPHCORE_API FVoxelBufferScatterer
{
public:
	static TSharedRef<const FVoxelBuffer> Scatter(
		const FVoxelBuffer& BaseBuffer,
		int32 BaseBufferNum,
		const FVoxelBuffer& BufferToScatter,
		const FVoxelInt32Buffer& Indices);

	template<typename T>
	static TSharedRef<const T> Scatter(
		const T& BaseBuffer,
		const int32 BaseBufferNum,
		const T& BufferToScatter,
		const FVoxelInt32Buffer& Indices)
	{
		return CastChecked<T>(Scatter(
			static_cast<const FVoxelBuffer&>(BaseBuffer),
			BaseBufferNum,
			static_cast<const FVoxelBuffer&>(BufferToScatter),
			Indices));
	}

private:
	static void Scatter(
		FVoxelTerminalBuffer& OutBuffer,
		const FVoxelTerminalBuffer& BaseBuffer,
		int32 BaseBufferNum,
		const FVoxelTerminalBuffer& BufferToScatter,
		const FVoxelInt32Buffer& Indices);
};