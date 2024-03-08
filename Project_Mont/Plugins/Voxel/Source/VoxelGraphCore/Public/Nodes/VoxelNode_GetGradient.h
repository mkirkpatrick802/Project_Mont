// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "VoxelNode_GetGradient.generated.h"

// Returns the unnormalized gradient of a value
// Will make any node before this 6x more expensive, use with caution
USTRUCT(Category = "Gradient")
struct VOXELGRAPHCORE_API FVoxelNode_GetGradient : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelFloatBuffer, Value, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelVectorBuffer, Gradient);
};