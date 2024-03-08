// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphSchemaAction.h"

class UVoxelGraph;
class UVoxelGraphNode_CallFunction;

struct FVoxelGraphSchemaAction_ExpandFunction : public FVoxelGraphSchemaAction
{
public:
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	//~ Begin FVoxelGraphSchemaAction Interface
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	//~ End FVoxelGraphSchemaAction Interface

private:
	void FindNearestSuitableLocation(const TSharedPtr<SGraphEditor>& GraphEditor, const UVoxelGraphNode_CallFunction* FunctionNode, const UVoxelTerminalGraph* FunctionGraph);
	void GroupNodesToCopy(const UEdGraph& FunctionEdGraph);
	void ConnectNewNodes(const UVoxelGraphNode_CallFunction* FunctionNode);

private:
	void ExportNodes(FString& ExportText) const;
	void ImportNodes(const TSharedPtr<SGraphEditor>& GraphEditor, UEdGraph* DestGraph, const FString& ExportText);

private:
	struct FCopiedNode
	{
		TWeakObjectPtr<UEdGraphNode> OriginalNode;
		TWeakObjectPtr<UEdGraphNode> NewNode;
		TMap<FEdGraphPinReference, TSet<FGuid>> MappedOriginalPinsToInputsOutputs;
	};

	FIntPoint SuitablePosition;
	FSlateRect PastedNodesBounds = FSlateRect(FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX);
	TMap<FGuid, TSharedPtr<FCopiedNode>> CopiedNodes;
};