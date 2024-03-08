// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_Output.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_Output : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	UPROPERTY()
	FGuid Guid;

	VOXEL_TEMPLATE_INPUT_PIN(FVoxelWildcard, Value, nullptr);

#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override
	{
		return FVoxelPinTypeSet::All();
	}
#endif
};