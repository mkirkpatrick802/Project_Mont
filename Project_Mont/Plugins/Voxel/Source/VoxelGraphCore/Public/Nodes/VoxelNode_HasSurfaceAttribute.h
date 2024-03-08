// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurface.h"
#include "VoxelNode_HasSurfaceAttribute.generated.h"

USTRUCT(Category = "Surface")
struct VOXELGRAPHCORE_API FVoxelNode_HasSurfaceAttribute : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FName, Attribute, "Material");
	VOXEL_OUTPUT_PIN(bool, ReturnValue);
};