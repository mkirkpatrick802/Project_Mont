// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelInputNodes.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_Input_WithoutDefaultPin : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	bool bIsGraphInput = false;

	UPROPERTY()
	FVoxelPinValue DefaultValue;

	VOXEL_TEMPLATE_OUTPUT_PIN(FVoxelWildcard, Value);

#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override
	{
		return FVoxelPinTypeSet::All();
	}
#endif

	//~ Begin FVoxelNode Interface
	virtual void PreCompile() override;
	//~ End FVoxelNode Interface

public:
	FVoxelRuntimePinValue RuntimeDefaultValue;
};

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_Input_WithDefaultPin : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	bool bIsGraphInput = false;

	VOXEL_TEMPLATE_INPUT_PIN(FVoxelWildcard, Default, nullptr);
	VOXEL_TEMPLATE_OUTPUT_PIN(FVoxelWildcard, Value);

#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override
	{
		return FVoxelPinTypeSet::All();
	}
#endif
};