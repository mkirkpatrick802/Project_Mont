// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_RangeDebug.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_RangeDebug : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelWildcard, In, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelWildcard, Out);

	FName RefPin;
};