// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelFloatBuffers.h"

struct VOXELGRAPHCORE_API FVoxelBufferTransformUtilities
{
public:
	static FVoxelVectorBuffer ApplyTransform(const FVoxelVectorBuffer& Buffer, const FTransform& Transform);
	static FVoxelVectorBuffer ApplyInverseTransform(const FVoxelVectorBuffer& Buffer, const FTransform& Transform);

	static FVoxelVectorBuffer ApplyTransform(
		const FVoxelVectorBuffer& Buffer,
		const FVoxelVectorBuffer& Translation,
		const FVoxelQuaternionBuffer& Rotation,
		const FVoxelVectorBuffer& Scale);

	static FVoxelVectorBuffer ApplyTransform(const FVoxelVectorBuffer& Buffer, const FMatrix& Transform);

public:
	static FVoxelFloatBuffer TransformDistance(const FVoxelFloatBuffer& Distance, const FMatrix& Transform);
};