// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Buffer/VoxelFloatBuffers.h"

struct VOXELGRAPHCORE_API FVoxelBufferMathUtilities
{
public:
	static FVoxelFloatBuffer Min8(
		const FVoxelFloatBuffer& Buffer0,
		const FVoxelFloatBuffer& Buffer1,
		const FVoxelFloatBuffer& Buffer2,
		const FVoxelFloatBuffer& Buffer3,
		const FVoxelFloatBuffer& Buffer4,
		const FVoxelFloatBuffer& Buffer5,
		const FVoxelFloatBuffer& Buffer6,
		const FVoxelFloatBuffer& Buffer7);

	static FVoxelFloatBuffer Max8(
		const FVoxelFloatBuffer& Buffer0,
		const FVoxelFloatBuffer& Buffer1,
		const FVoxelFloatBuffer& Buffer2,
		const FVoxelFloatBuffer& Buffer3,
		const FVoxelFloatBuffer& Buffer4,
		const FVoxelFloatBuffer& Buffer5,
		const FVoxelFloatBuffer& Buffer6,
		const FVoxelFloatBuffer& Buffer7);

	static FVoxelVectorBuffer Min8(
		const FVoxelVectorBuffer& Buffer0,
		const FVoxelVectorBuffer& Buffer1,
		const FVoxelVectorBuffer& Buffer2,
		const FVoxelVectorBuffer& Buffer3,
		const FVoxelVectorBuffer& Buffer4,
		const FVoxelVectorBuffer& Buffer5,
		const FVoxelVectorBuffer& Buffer6,
		const FVoxelVectorBuffer& Buffer7);

	static FVoxelVectorBuffer Max8(
		const FVoxelVectorBuffer& Buffer0,
		const FVoxelVectorBuffer& Buffer1,
		const FVoxelVectorBuffer& Buffer2,
		const FVoxelVectorBuffer& Buffer3,
		const FVoxelVectorBuffer& Buffer4,
		const FVoxelVectorBuffer& Buffer5,
		const FVoxelVectorBuffer& Buffer6,
		const FVoxelVectorBuffer& Buffer7);

public:
	static FVoxelFloatBuffer Add(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B);
	static FVoxelFloatBuffer Multiply(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B);
	static FVoxelBoolBuffer Less(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B);

public:
	static FVoxelVectorBuffer Add(const FVoxelVectorBuffer& A, const FVoxelVectorBuffer& B);
	static FVoxelVectorBuffer Multiply(const FVoxelVectorBuffer& A, const FVoxelVectorBuffer& B);

public:
	static FVoxelFloatBuffer Lerp(const FVoxelFloatBuffer& A, const FVoxelFloatBuffer& B, const FVoxelFloatBuffer& Alpha);
	static FVoxelVector2DBuffer Lerp(const FVoxelVector2DBuffer& A, const FVoxelVector2DBuffer& B, const FVoxelFloatBuffer& Alpha);
	static FVoxelVectorBuffer Lerp(const FVoxelVectorBuffer& A, const FVoxelVectorBuffer& B, const FVoxelFloatBuffer& Alpha);
	static FVoxelLinearColorBuffer Lerp(const FVoxelLinearColorBuffer& A, const FVoxelLinearColorBuffer& B, const FVoxelFloatBuffer& Alpha);

public:
	static FVoxelQuaternionBuffer Combine(const FVoxelQuaternionBuffer& A, const FVoxelQuaternionBuffer& B);
	static FVoxelVectorBuffer MakeEulerFromQuaternion(const FVoxelQuaternionBuffer& Quaternion);
};