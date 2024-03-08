// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphCollapseToFunctionSchemaAction.h"

#include "VoxelEdGraph.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphToolkit.h"
#include "Nodes/VoxelGraphNode.h"
#include "Nodes/VoxelGraphNode_Input.h"
#include "Nodes/VoxelGraphNode_Output.h"
#include "Nodes/VoxelGraphNode_CallFunction.h"
#include "Nodes/VoxelGraphNode_LocalVariable.h"

#include "SNodePanel.h"

UEdGraphNode* FVoxelGraphSchemaAction_CollapseToFunction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = FVoxelGraphToolkit::Get(ParentGraph);
	if (!ensure(Toolkit))
	{
		return nullptr;
	}

	const TSharedPtr<SGraphEditor> GraphEditor = Toolkit->FindGraphEditor(*ParentGraph);
	if (!ensure(GraphEditor))
	{
		return nullptr;
	}

	const FVoxelTransaction Transaction(Toolkit->Asset, "Collapse nodes to Function");

	UVoxelTerminalGraph* NewFunctionGraph;
	{
		FVoxelGraphSchemaAction_NewFunction Action;
		// If we open newly created function graph, undo action break parent graph and does not show recreated nodes
		Action.bOpenNewGraph = false;
		Action.PerformAction(Toolkit->GetActiveEdGraph(), nullptr, FVector2D::ZeroVector);
		NewFunctionGraph = Action.OutNewFunction.Get();
		if (!ensure(NewFunctionGraph))
		{
			return nullptr;
		}

		Cast<UVoxelEdGraph>(&NewFunctionGraph->GetEdGraph())->SetToolkit(Toolkit.ToSharedRef());
	}

	if (!ensure(NewFunctionGraph))
	{
		return nullptr;
	}

	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	GroupSelectedNodes(SelectedNodes);

	// Copy and Paste
	{
		FString ExportedText;
		ExportNodes(ExportedText);
		ImportNodes(NewFunctionGraph, ExportedText);
	}

	AddLocalVariableInputs(NewFunctionGraph);
	AddDeclarationOutputs(NewFunctionGraph);
	AddOuterInputsOutputs(NewFunctionGraph);

	UVoxelGraphNode_CallFunction* NewFunctionNode;
	{
		FVoxelGraphSchemaAction_NewCallFunctionNode Action;
		Action.Guid = NewFunctionGraph->GetGuid();
		NewFunctionNode = Cast<UVoxelGraphNode_CallFunction>(Action.PerformAction(ParentGraph, nullptr, AvgNodePosition));
	}

	if (!ensure(NewFunctionNode))
	{
		return nullptr;
	}

	FixupMainGraph(NewFunctionNode, ParentGraph);

	GraphEditor->NotifyGraphChanged();

	return NewFunctionNode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphSchemaAction_CollapseToFunction::GroupSelectedNodes(const TSet<UObject*>& SelectedNodes)
{
	for (UObject* SelectedNode : SelectedNodes)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(SelectedNode);
		if (!ensure(Node))
		{
			continue;
		}

		TMap<FEdGraphPinReference, TSet<FEdGraphPinReference>> OuterPins;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin->HasAnyConnections())
			{
				continue;
			}

			for (const UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (!ensure(LinkedTo))
				{
					continue;
				}

				const UEdGraphNode* LinkedToNode = LinkedTo->GetOwningNode();
				if (!ensure(LinkedToNode))
				{
					continue;
				}

				if (!SelectedNodes.Contains(LinkedToNode))
				{
					OuterPins.FindOrAdd(Pin).Add(LinkedTo);
				}
			}
		}

		TSharedRef<FCopiedNode> CopiedNode = MakeShared<FCopiedNode>(Node, OuterPins);
		CopiedNodes.Add(Node->NodeGuid, CopiedNode);

		if (const UVoxelGraphNode_LocalVariable* LocalVariableNode = Cast<UVoxelGraphNode_LocalVariable>(Node))
		{
			TSharedPtr<FCopiedLocalVariable> CopiedLocalVariable = CopiedLocalVariables.FindRef(LocalVariableNode->Guid);
			if (!CopiedLocalVariable)
			{
				CopiedLocalVariable = MakeShared<FCopiedLocalVariable>(LocalVariableNode->Guid);
				CopiedLocalVariables.Add(LocalVariableNode->Guid, CopiedLocalVariable);
			}

			if (LocalVariableNode->IsA<UVoxelGraphNode_LocalVariableDeclaration>())
			{
				CopiedLocalVariable->DeclarationNode = CopiedNode;
			}
			else
			{
				CopiedLocalVariable->UsageNodes.Add(CopiedNode);
			}
		}
	}
}

void FVoxelGraphSchemaAction_CollapseToFunction::AddLocalVariableInputs(UVoxelTerminalGraph* Graph)
{
	for (const auto& It : CopiedLocalVariables)
	{
		if (It.Value->DeclarationNode)
		{
			continue;
		}

		const FVoxelGraphLocalVariable* LocalVariable = Graph->FindLocalVariable(It.Value->NewGuid);
		if (!ensure(LocalVariable))
		{
			continue;
		}

		FVector2D Position = InputDeclarationPosition;
		InputDeclarationPosition.Y += 250.f;

		// We create function inputs for local variables and assign them to newly created local variables, to use them in multiple places
		UVoxelGraphNode_Input* FunctionInput;
		{
			FVoxelGraphSchemaAction_NewFunctionInput Action;
			Action.TypeOverride = LocalVariable->Type;
			Action.NameOverride = LocalVariable->Name;
			Action.Category = LocalVariable->Category;

			FunctionInput = Cast<UVoxelGraphNode_Input>(Action.PerformAction(&Graph->GetEdGraph(), nullptr, Position, false));
		}

		if (!ensure(FunctionInput))
		{
			continue;
		}

		UVoxelGraphNode_LocalVariableDeclaration* LocalVariableDeclaration;
		{
			FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
			Action.Type = LocalVariable->Type;
			Action.Guid = It.Value->NewGuid;
			Action.bIsDeclaration = true;

			Position.X += 350.f;

			LocalVariableDeclaration = Cast<UVoxelGraphNode_LocalVariableDeclaration>(Action.PerformAction(&Graph->GetEdGraph(), FunctionInput->GetOutputPin(0), Position, false));
		}

		if (!ensure(LocalVariableDeclaration))
		{
			FunctionInput->DestroyNode();
			continue;
		}

		It.Value->InputGuid = FunctionInput->Guid;
	}
}

void FVoxelGraphSchemaAction_CollapseToFunction::AddDeclarationOutputs(UVoxelTerminalGraph* Graph)
{
	for (const auto& It : CopiedLocalVariables)
	{
		if (!It.Value->DeclarationNode)
		{
			continue;
		}

		const FVoxelGraphLocalVariable* LocalVariable = Graph->FindLocalVariable(It.Value->NewGuid);
		if (!ensure(LocalVariable))
		{
			continue;
		}

		FVector2D Position = OutputDeclarationPosition;
		OutputDeclarationPosition.Y += 250.f;

		UVoxelGraphNode_LocalVariableUsage* LocalVariableUsage;
		{
			FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
			Action.Type = LocalVariable->Type;
			Action.Guid = It.Value->NewGuid;
			Action.bIsDeclaration = false;

			LocalVariableUsage = Cast<UVoxelGraphNode_LocalVariableUsage>(Action.PerformAction(&Graph->GetEdGraph(), nullptr, Position, false));
		}

		if (!ensure(LocalVariableUsage))
		{
			continue;
		}

		// We create function outputs for local variables and assign them to newly created local variables, to use them in multiple places
		UVoxelGraphNode_Output* FunctionOutput;
		{
			FVoxelGraphSchemaAction_NewOutput Action;
			Action.TypeOverride = LocalVariable->Type;
			Action.NameOverride = FName("Out" + LocalVariable->Name.ToString());
			Action.Category = LocalVariable->Category;

			Position.X += 350.f;

			FunctionOutput = Cast<UVoxelGraphNode_Output>(Action.PerformAction(&Graph->GetEdGraph(), LocalVariableUsage->GetOutputPin(0), Position, false));
		}

		if (!ensure(FunctionOutput))
		{
			LocalVariableUsage->DestroyNode();
			continue;
		}

		It.Value->OutputGuid = FunctionOutput->Guid;
	}
}

void FVoxelGraphSchemaAction_CollapseToFunction::AddOuterInputsOutputs(UVoxelTerminalGraph* Graph)
{
	for (const auto& It : CopiedNodes)
	{
		if (!ensure(It.Value->OriginalNode.Get()) ||
			!ensure(It.Value->NewNode.Get()))
		{
			continue;
		}

		const UVoxelGraphNode* NewNode = It.Value->GetNewNode<UVoxelGraphNode>();
		const UVoxelGraphNode* OriginalNode = It.Value->GetOriginalNode<UVoxelGraphNode>();
		if (!NewNode ||
			!OriginalNode)
		{
			continue;
		}

		for (auto& OuterPinIt : It.Value->OutsideConnectedPins)
		{
			const UEdGraphPin* OriginalPin = OuterPinIt.Key.Get();
			if (!ensure(OriginalPin))
			{
				continue;
			}

			const FName TargetPin = OriginalPin->PinName;

			UEdGraphPin* NewPin = NewNode->FindPin(TargetPin, OriginalPin->Direction);
			if (!ensure(NewPin))
			{
				continue;
			}

			// We create function inputs/outputs for pins which were connected to outer nodes
			FGuid InputOutputGuid;
			for (const FEdGraphPinReference& WeakOuterPin : OuterPinIt.Value)
			{
				const UEdGraphPin* OuterPin = WeakOuterPin.Get();
				if (!ensure(OuterPin))
				{
					continue;
				}

				UEdGraphNode* OuterNode = OuterPin->GetOwningNode();
				if (!ensure(OuterNode))
				{
					continue;
				}

				// If pin is input and does reference already created parameter, connect it to its usage
				if (NewPin->Direction == EGPD_Input)
				{
					if (const UVoxelGraphNode_LocalVariable* OuterParameterNode = Cast<UVoxelGraphNode_LocalVariable>(OuterNode))
					{
						if (const TSharedPtr<FCopiedLocalVariable>& CopiedParameter = CopiedLocalVariables.FindRef(OuterParameterNode->Guid))
						{
							FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
							Action.bIsDeclaration = false;
							Action.Guid = CopiedParameter->NewGuid;
							Action.PerformAction(&Graph->GetEdGraph(), NewPin, GetNodePosition(OuterParameterNode), false);
							continue;
						}
					}
				}

				if (!InputOutputGuid.IsValid())
				{
					FVector2D Position(NewNode->NodePosX, NewNode->NodePosY - 200.f);
					if (NewPin->Direction == EGPD_Input)
					{
						FVoxelGraphSchemaAction_NewFunctionInput Action;
						Action.TypeOverride = NewPin->PinType;
						Position.X -= 350.f;
						Position.Y += NewNode->GetInputPins().IndexOfByKey(NewPin) * 100.f;
						const UVoxelGraphNode_Input* InputNode = Cast<UVoxelGraphNode_Input>(Action.PerformAction(&Graph->GetEdGraph(), NewPin, Position, false));
						if (ensure(InputNode))
						{
							InputOutputGuid = InputNode->Guid;
						}
					}
					else
					{
						FVoxelGraphSchemaAction_NewOutput Action;
						Action.TypeOverride = NewPin->PinType;
						Position.X += 350.f;
						Position.Y += NewNode->GetOutputPins().IndexOfByKey(NewPin) * 100.f;
						const UVoxelGraphNode_Output* OutputNode = Cast<UVoxelGraphNode_Output>(Action.PerformAction(&Graph->GetEdGraph(), NewPin, Position, false));
						if (ensure(OutputNode))
						{
							InputOutputGuid = OutputNode->Guid;
						}
					}
				}

				if (!ensure(InputOutputGuid.IsValid()))
				{
					continue;
				}

				It.Value->MappedInputsOutputs.FindOrAdd(InputOutputGuid, {}).Add(WeakOuterPin);
			}
		}
	}
}

void FVoxelGraphSchemaAction_CollapseToFunction::FixupMainGraph(const UVoxelGraphNode_CallFunction* FunctionNode, UEdGraph* EdGraph)
{
	// Remove all original nodes
	{
		for (const auto& It : CopiedNodes)
		{
			UEdGraphNode* Node = It.Value->GetOriginalNode<UEdGraphNode>();
			if (!ensure(Node))
			{
				continue;
			}

			Node->DestroyNode();
		}
	}

	// Create outer node links
	{
		for (const auto& It : CopiedNodes)
		{
			for (auto& OuterLinkIt : It.Value->MappedInputsOutputs)
			{
				UEdGraphPin* FunctionPin = FunctionNode->FindPin(OuterLinkIt.Key.ToString());
				if (!ensure(FunctionPin))
				{
					continue;
				}

				for (const FEdGraphPinReference& WeakOuterPin : OuterLinkIt.Value)
				{
					UEdGraphPin* OuterPin = WeakOuterPin.Get();
					if (!ensure(OuterPin))
					{
						continue;
					}

					FunctionPin->MakeLinkTo(OuterPin);
				}
			}
		}
	}

	// Link parameters/local variables
	{
		for (const auto& It : CopiedLocalVariables)
		{
			if (It.Value->InputGuid.IsValid())
			{
				UEdGraphPin* InputPin = FunctionNode->FindPin(It.Value->InputGuid.ToString());
				if (!ensure(InputPin))
				{
					continue;
				}

				if (InputPin->LinkedTo.Num() > 0)
				{
					continue;
				}

				FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
				Action.Guid = It.Value->OldGuid;
				Action.bIsDeclaration = false;

				FVector2D Position(AvgNodePosition.X - 200.f, AvgNodePosition.Y);
				Position.Y += FunctionNode->GetInputPins().IndexOfByKey(InputPin) * 100.f;

				ensure(Action.PerformAction(EdGraph, InputPin, Position, false));
			}

			if (It.Value->OutputGuid.IsValid())
			{
				UEdGraphPin* OutputPin = FunctionNode->FindPin(It.Value->OutputGuid.ToString());
				if (!ensure(OutputPin))
				{
					continue;
				}

				if (OutputPin->LinkedTo.Num() > 0)
				{
					continue;
				}

				FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
				Action.Guid = It.Value->OldGuid;
				Action.bIsDeclaration = true;

				FVector2D Position(AvgNodePosition.X + 200.f, AvgNodePosition.Y);
				Position.Y += FunctionNode->GetInputPins().IndexOfByKey(OutputPin) * 100.f;

				ensure(Action.PerformAction(EdGraph, OutputPin, Position, false));
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphSchemaAction_CollapseToFunction::ExportNodes(FString& ExportText) const
{
	TSet<UObject*> Nodes;
	for (const auto& It : CopiedNodes)
	{
		UEdGraphNode* Node = It.Value->OriginalNode.Get();
		if (!ensure(Node) ||
			!Node->CanDuplicateNode())
		{
			continue;
		}

		Node->PrepareForCopying();
		Nodes.Add(Node);
	}

	FEdGraphUtilities::ExportNodesToText(Nodes, ExportText);
}

void FVoxelGraphSchemaAction_CollapseToFunction::ImportNodes(UVoxelTerminalGraph* Graph, const FString& ExportText)
{
	const FVoxelTransaction Transaction(Graph, "Paste nodes");

	TSet<UEdGraphNode*> PastedNodes;

	// Import the nodes
	FEdGraphUtilities::ImportNodesFromText(&Graph->GetEdGraph(), ExportText, PastedNodes);

	// Average position of nodes so we can move them while still maintaining relative distances to each other
	AvgNodePosition = FVector2D::ZeroVector;

	for (auto It = PastedNodes.CreateIterator(); It; ++It)
	{
		UEdGraphNode* Node = *It;
		if (!ensure(Node))
		{
			It.RemoveCurrent();
			continue;
		}

		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;

		UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node);
		if (!VoxelNode ||
			VoxelNode->CanPasteVoxelNode(PastedNodes))
		{
			continue;
		}

		VoxelNode->DestroyNode();
		It.RemoveCurrent();
	}

	if (PastedNodes.Num() > 0)
	{
		AvgNodePosition.X /= PastedNodes.Num();
		AvgNodePosition.Y /= PastedNodes.Num();
	}

	// Top left corner of all nodes
	InputDeclarationPosition.X = MAX_FLT;
	InputDeclarationPosition.Y = MAX_FLT;

	// Top right corner of all nodes
	OutputDeclarationPosition.X = -MAX_FLT;
	OutputDeclarationPosition.Y = MAX_FLT;

	for (UEdGraphNode* Node : PastedNodes)
	{
		const FVector2D NodePosition = GetNodePosition(Node);
		Node->NodePosX = NodePosition.X;
		Node->NodePosY = NodePosition.Y;

		InputDeclarationPosition.X = FMath::Min(Node->NodePosX, InputDeclarationPosition.X);
		InputDeclarationPosition.Y = FMath::Min(Node->NodePosY, InputDeclarationPosition.Y);

		OutputDeclarationPosition.X = FMath::Max(Node->NodePosX, OutputDeclarationPosition.X);
		OutputDeclarationPosition.Y = FMath::Min(Node->NodePosY, OutputDeclarationPosition.Y);

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());

		const TSharedPtr<FCopiedNode> CopiedNode = CopiedNodes.FindRef(Node->NodeGuid);
		if (ensure(CopiedNode))
		{
			CopiedNode->NewNode = Node;

			if (const UVoxelGraphNode_LocalVariable* OriginalLocalVariableNode = CopiedNode->GetOriginalNode<UVoxelGraphNode_LocalVariable>())
			{
				const TSharedPtr<FCopiedLocalVariable> CopiedLocalVariable = CopiedLocalVariables.FindRef(OriginalLocalVariableNode->Guid);
				const UVoxelGraphNode_LocalVariable* NewLocalVariableNode = Cast<UVoxelGraphNode_LocalVariable>(Node);
				if (ensure(CopiedLocalVariable) &&
					ensure(NewLocalVariableNode))
				{
					CopiedLocalVariable->NewGuid = NewLocalVariableNode->Guid;
				}
			}
		}

		// Give new node a different GUID from the old one
		Node->CreateNewGuid();
	}

	// Post paste for local variables
	for (UEdGraphNode* Node : PastedNodes)
	{
		if (UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node))
		{
			VoxelNode->PostPasteVoxelNode(PastedNodes);
		}
	}

	InputDeclarationPosition.X -= 800.f;
	OutputDeclarationPosition.X += 800.f;
}

FVector2D FVoxelGraphSchemaAction_CollapseToFunction::GetNodePosition(const UEdGraphNode* Node) const
{
	return FVector2D(Node->NodePosX - AvgNodePosition.X, Node->NodePosY - AvgNodePosition.Y);
}