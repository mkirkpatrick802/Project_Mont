// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphSchemaAction.h"

class UVoxelGraphNode_CallFunction;

struct FVoxelGraphSchemaAction_CollapseToFunction : public FVoxelGraphSchemaAction
{
public:
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	//~ Begin FVoxelGraphSchemaAction Interface
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	//~ End FVoxelGraphSchemaAction Interface

private:
	void GroupSelectedNodes(const TSet<UObject*>& SelectedNodes);
	void AddLocalVariableInputs(UVoxelTerminalGraph* Graph);
	void AddDeclarationOutputs(UVoxelTerminalGraph* Graph);
	void AddOuterInputsOutputs(UVoxelTerminalGraph* Graph);
	void FixupMainGraph(const UVoxelGraphNode_CallFunction* FunctionNode, UEdGraph* EdGraph);

private:
	void ExportNodes(FString& ExportText) const;
	void ImportNodes(UVoxelTerminalGraph* Graph, const FString& ExportText);
	FVector2D GetNodePosition(const UEdGraphNode* Node) const;

private:
	struct FCopiedNode
	{
		TWeakObjectPtr<UEdGraphNode> OriginalNode;
		TWeakObjectPtr<UEdGraphNode> NewNode;
		TMap<FEdGraphPinReference, TSet<FEdGraphPinReference>> OutsideConnectedPins;
		TMap<FGuid, TSet<FEdGraphPinReference>> MappedInputsOutputs;

		FCopiedNode(UEdGraphNode* Node, const TMap<FEdGraphPinReference, TSet<FEdGraphPinReference>>& OutsideConnectedPins)
			: OriginalNode(Node)
			, OutsideConnectedPins(OutsideConnectedPins)
		{
		}

		template<typename T>
		T* GetOriginalNode()
		{
			return Cast<T>(OriginalNode.Get());
		}

		template<typename T>
		T* GetNewNode()
		{
			return Cast<T>(NewNode.Get());
		}
	};

	struct FCopiedLocalVariable
	{
		FGuid NewGuid;
		FGuid OldGuid;

		FGuid InputGuid;
		FGuid OutputGuid;

		TSharedPtr<FCopiedNode> DeclarationNode;
		TSet<TSharedPtr<FCopiedNode>> UsageNodes;

		explicit FCopiedLocalVariable(const FGuid OriginalParameterId)
			: OldGuid(OriginalParameterId)
		{}
	};

	TMap<FGuid, TSharedPtr<FCopiedNode>> CopiedNodes;
	TMap<FGuid, TSharedPtr<FCopiedLocalVariable>> CopiedLocalVariables;

	FVector2D AvgNodePosition;
	FVector2D InputDeclarationPosition;
	FVector2D OutputDeclarationPosition;
};