// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_AppendNames.generated.h"

USTRUCT(Category = "Name", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_AppendNames : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_VARIADIC_INPUT_PIN(FName, Names, nullptr, 2);
	VOXEL_OUTPUT_PIN(FName, Name);

	//~ Begin FVoxelNode Interface
	virtual bool IsPureNode() const override
	{
		return true;
	}
	//~ End FVoxelNode Interface
};