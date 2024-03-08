// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_Root.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_Root : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	// Use the same pin name as the pin we're cloning to have accurate callstacks
	using Super::CreateInputPin;
};