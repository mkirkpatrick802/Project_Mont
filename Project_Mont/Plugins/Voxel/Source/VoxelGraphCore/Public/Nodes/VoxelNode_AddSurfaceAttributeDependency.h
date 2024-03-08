// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurface.h"
#include "VoxelNode_AddSurfaceAttributeDependency.generated.h"

// Tag an attribute as requiring another attribute to be computed
// Typically, computing Material often requires computing Distance as well
USTRUCT(Category = "Surface", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_AddSurfaceAttributeDependency : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, In, nullptr);
	VOXEL_INPUT_PIN(FName, Attribute, FVoxelSurfaceAttributes::Material);
	VOXEL_INPUT_PIN(FName, Dependency, FVoxelSurfaceAttributes::Distance);
	VOXEL_OUTPUT_PIN(FVoxelSurface, Out);
};