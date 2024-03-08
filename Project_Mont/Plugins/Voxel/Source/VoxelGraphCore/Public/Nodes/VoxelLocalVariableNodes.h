// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelLocalVariableNodes.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_LocalVariableDeclaration : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	UPROPERTY()
	FGuid Guid;

	VOXEL_INPUT_PIN(FVoxelWildcard, InputPin, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelWildcard, OutputPin);
};

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_LocalVariableUsage : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	UPROPERTY()
	FGuid Guid;

	VOXEL_OUTPUT_PIN(FVoxelWildcard, OutputPin);
};