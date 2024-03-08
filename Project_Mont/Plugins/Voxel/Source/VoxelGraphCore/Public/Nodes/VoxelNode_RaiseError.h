// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_RaiseError.generated.h"

// Will raise an error whenever the output pin is queried
USTRUCT(Category = "Misc")
struct VOXELGRAPHCORE_API FVoxelNode_RaiseError : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelWildcard, In, nullptr);
	VOXEL_INPUT_PIN(FName, Error, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelWildcard, Out);

#if WITH_EDITOR
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif
};