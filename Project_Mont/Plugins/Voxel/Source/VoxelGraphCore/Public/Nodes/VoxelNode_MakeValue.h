// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_MakeValue.generated.h"

USTRUCT(Category = "Misc", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_MakeValue : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelWildcard, Value);

	//~ Begin FVoxelNode Interface
	virtual bool IsPureNode() const override
	{
		return true;
	}

#if WITH_EDITOR
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const override;
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType) override;
#endif

	virtual void PreCompile() override;
	virtual void PostSerialize() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End FVoxelNode Interface

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelPinValue Value;
	FVoxelRuntimePinValue RuntimeValue;

	void FixupValue();
};