// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Point/VoxelPointSet.h"
#include "VoxelNode_MergePoints.generated.h"

USTRUCT(Category = "Point")
struct VOXELGRAPHCORE_API FVoxelNode_MergePoints : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_VARIADIC_INPUT_PIN(FVoxelPointSet, Input, nullptr, 2);
	VOXEL_OUTPUT_PIN(FVoxelPointSet, Out);
};