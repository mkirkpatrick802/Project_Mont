// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Point/VoxelPointSet.h"
#include "VoxelNode_PruneByDistance.generated.h"

// Will prune any points closer to each others than Distance
USTRUCT(Category = "Point")
struct VOXELGRAPHCORE_API FVoxelNode_PruneByDistance : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelPointSet, In, nullptr);
	VOXEL_INPUT_PIN(float, Distance, 100.f);
	VOXEL_OUTPUT_PIN(FVoxelPointSet, Out);
};