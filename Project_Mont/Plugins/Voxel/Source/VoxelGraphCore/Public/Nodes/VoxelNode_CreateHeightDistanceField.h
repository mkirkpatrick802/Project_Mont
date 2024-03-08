// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelNode_CreateHeightDistanceField.generated.h"

// Make a distance field from 2D height data
// Typically, a heightmap
USTRUCT(Category = "Distance Field", meta = (Keywords = "make"))
struct VOXELGRAPHCORE_API FVoxelNode_CreateHeightDistanceField : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Height, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelFloatBuffer, Distance);
};