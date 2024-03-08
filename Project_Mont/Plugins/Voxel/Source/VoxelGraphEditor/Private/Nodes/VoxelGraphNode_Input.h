// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTerminalGraph.h"
#include "Nodes/VoxelGraphNode.h"
#include "VoxelGraphNode_Input.generated.h"

UCLASS()
class UVoxelGraphNode_Input : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	bool bExposeDefaultPin = false;

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	bool bIsGraphInput = false;

	UPROPERTY()
	FVoxelGraphInput CachedInput;

	UVoxelTerminalGraph* GetTerminalGraph() const;
	const FVoxelGraphInput* GetInput() const;
	FVoxelGraphInput GetInputSafe() const;

	//~ Begin UVoxelGraphNode Interface
	virtual bool IsVariable() const override { return true; }
	virtual void AllocateDefaultPins() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetSearchTerms() const override;
	//~ End UVoxelGraphNode Interface

private:
	FSharedVoidPtr OnInputChangedPtr;
};