// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelBaseBuffers.h"

struct VOXELGRAPHCORE_API FVoxelBufferSelector
{
public:
	static TSharedRef<const FVoxelBuffer> SelectGeneric(
		const FVoxelPinType& InnerType,
		const FVoxelBuffer& Indices,
		TConstVoxelArrayView<const FVoxelBuffer*> Buffers);

public:
	static TSharedRef<const FVoxelBuffer> Select(
		const FVoxelPinType& InnerType,
		const FVoxelBoolBuffer& Condition,
		const FVoxelBuffer& FalseBuffer,
		const FVoxelBuffer& TrueBuffer);

	static TSharedRef<const FVoxelBuffer> Select(
		const FVoxelPinType& InnerType,
		const FVoxelByteBuffer& Indices,
		TConstVoxelArrayView<const FVoxelBuffer*> Buffers);

	static TSharedRef<const FVoxelBuffer> Select(
		const FVoxelPinType& InnerType,
		const FVoxelInt32Buffer& Indices,
		TConstVoxelArrayView<const FVoxelBuffer*> Buffers);

private:
	static bool Check(
		const FVoxelPinType& InnerType,
		const FVoxelBuffer& Indices,
		TConstVoxelArrayView<const FVoxelBuffer*> Buffers);

	static void Select_Bool(
		FVoxelTerminalBuffer& OutBuffer,
		const FVoxelBoolBuffer& Condition,
		const FVoxelBuffer& FalseBuffer,
		const FVoxelBuffer& TrueBuffer);

	static void Select_Byte(
		FVoxelTerminalBuffer& OutBuffer,
		const FVoxelByteBuffer& Indices,
		TConstVoxelArrayView<const FVoxelTerminalBuffer*> Buffers);

	static void Select_Int32(
		FVoxelTerminalBuffer& OutBuffer,
		const FVoxelInt32Buffer& Indices,
		TConstVoxelArrayView<const FVoxelTerminalBuffer*> Buffers);
};