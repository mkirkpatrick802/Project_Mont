// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Nodes/VoxelGraphNode.h"
#include "VoxelGraphNode_CallTerminalGraph.generated.h"

class UVoxelTerminalGraph;

UCLASS(Abstract)
class UVoxelGraphNode_CallTerminalGraph : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	virtual void AddPins() {}
	virtual const UVoxelTerminalGraph* GetBaseTerminalGraph() const VOXEL_PURE_VIRTUAL({});

	//~ Begin UVoxelGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }

	virtual bool IsPinOptional(const UEdGraphPin& Pin) const override;

	virtual bool CanJumpToDefinition() const override { return true; }
	virtual void JumpToDefinition() const override;

	virtual FName GetPinCategory(const UEdGraphPin& Pin) const override;
	virtual TSharedRef<IVoxelNodeDefinition> GetNodeDefinition() override;
	//~ End UVoxelGraphNode Interface

private:
	TSet<UEdGraphPin*> CachedOptionalPins;
	FSharedVoidPtr OnChangedPtr;
};

class FVoxelNodeDefinition_CallTerminalGraph : public IVoxelNodeDefinition
{
public:
	UVoxelGraphNode_CallTerminalGraph& Node;

	explicit FVoxelNodeDefinition_CallTerminalGraph(UVoxelGraphNode_CallTerminalGraph& Node)
		: Node(Node)
	{
	}

	//~ Begin IVoxelNodeDefinition Interface
	virtual TSharedPtr<const FNode> GetInputs() const override;
	virtual TSharedPtr<const FNode> GetOutputs() const override;

	virtual bool OverridePinsOrder() const override
	{
		return true;
	}
	//~ End IVoxelNodeDefinition Interface
};