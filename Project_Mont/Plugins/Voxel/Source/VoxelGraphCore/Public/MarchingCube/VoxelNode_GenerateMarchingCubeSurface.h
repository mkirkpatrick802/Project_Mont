// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Material/VoxelMaterial.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "MarchingCube/VoxelMarchingCubeSurface.h"
#include "VoxelNode_GenerateMarchingCubeSurface.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_GenerateMarchingCubeSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Distance, nullptr);
	VOXEL_INPUT_PIN(int32, VoxelSize, nullptr);
	VOXEL_INPUT_PIN(int32, ChunkSize, nullptr);
	VOXEL_INPUT_PIN(FVoxelBox, Bounds, nullptr);
	VOXEL_INPUT_PIN(bool, EnableTransitions, nullptr);
	VOXEL_INPUT_PIN(bool, PerfectTransitions, nullptr);
	VOXEL_INPUT_PIN(bool, EnableDistanceChecks, nullptr);
	VOXEL_INPUT_PIN(float, DistanceChecksTolerance, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMarchingCubeSurface, Surface);
};