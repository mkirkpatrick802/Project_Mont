// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelBaseBuffers.h"

struct FVoxelPointIdBuffer;

struct VOXELGRAPHCORE_API FVoxelBufferConversionUtilities
{
	static FVoxelFloatBuffer IntToFloat(const FVoxelInt32Buffer& Buffer);
	static FVoxelDoubleBuffer IntToDouble(const FVoxelInt32Buffer& Buffer);

	static FVoxelDoubleBuffer FloatToDouble(const FVoxelFloatBuffer& Float);
	static FVoxelFloatBuffer DoubleToFloat(const FVoxelDoubleBuffer& Double);

	static FVoxelSeedBuffer PointIdToSeed(const FVoxelPointIdBuffer& Buffer);
};