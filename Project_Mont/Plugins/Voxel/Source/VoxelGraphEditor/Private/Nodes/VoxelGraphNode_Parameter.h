// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelParameter.h"
#include "Nodes/VoxelGraphNode.h"
#include "VoxelGraphNode_Parameter.generated.h"

UCLASS()
class UVoxelGraphNode_Parameter : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bIsBuffer = false;

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FVoxelParameter CachedParameter;

	const FVoxelParameter* GetParameter() const;
	FVoxelParameter GetParameterSafe() const;

	//~ Begin UVoxelGraphNode Interface
	virtual bool IsVariable() const override { return true; }
	virtual void AllocateDefaultPins() override;

	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetSearchTerms() const override;

	virtual bool CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const override;
	virtual void PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType) override;
	//~ End UVoxelGraphNode Interface

private:
	FSharedVoidPtr OnParameterChangedPtr;
};