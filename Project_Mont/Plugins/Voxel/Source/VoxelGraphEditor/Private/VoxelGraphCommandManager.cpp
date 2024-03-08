// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphCommandManager.h"
#include "VoxelNodeStats.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "VoxelGraphSearchManager.h"
#include "VoxelTerminalGraphRuntime.h"
#include "Nodes/VoxelNode_RangeDebug.h"
#include "Nodes/VoxelGraphNode.h"
#include "Widgets/SVoxelGraphSearch.h"

DEFINE_VOXEL_COMMANDS(FVoxelGraphCommands);

void FVoxelGraphCommands::RegisterCommands()
{
	VOXEL_UI_COMMAND(Compile, "Compile", "Recompile the graph, done automatically whenever the graph is changed", EUserInterfaceActionType::Button, FInputChord());
	VOXEL_UI_COMMAND(CompileAll, "Compile All", "Recompile all graphs and open all the graphs with errors or warnings", EUserInterfaceActionType::Button, FInputChord());
	VOXEL_UI_COMMAND(EnableStats, "Enable Stats", "Enable stats", EUserInterfaceActionType::ToggleButton, FInputChord());
	VOXEL_UI_COMMAND(EnableRangeStats, "Enable Range Stats", "Analyses node outputs to find the lowest and highest values. This is based only on computed positions, and because of that may not be representative of the full range.\n\nNote: This increases generation times while enabled, and only works on integer and float-based values.", EUserInterfaceActionType::ToggleButton, FInputChord());
	VOXEL_UI_COMMAND(FindInGraph, "Find", "Finds references to functions, variables, and pins in the current Graph (use Ctrl+Shift+F to search in all Graphs)", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
	VOXEL_UI_COMMAND(FindInGraphs, "Find in Graphs", "Find references to functions and variables in ALL Graphs", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control | EModifierKey::Shift, EKeys::F));
	VOXEL_UI_COMMAND(ReconstructAllNodes, "Reconstruct all nodes", "Reconstructs all nodes in the graph", EUserInterfaceActionType::Button, FInputChord());
	VOXEL_UI_COMMAND(ToggleDebug, "Toggle Debug", "Toggle node debug", EUserInterfaceActionType::Button, FInputChord(EKeys::D));
	VOXEL_UI_COMMAND(TogglePreview, "Toggle Preview", "Toggle node preview", EUserInterfaceActionType::Button, FInputChord(EKeys::R));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCommandManager::MapActions() const
{
	VOXEL_FUNCTION_COUNTER();

	FGraphEditorCommands::Register();

	FUICommandList& Commands = *Toolkit.GetCommands();

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	Commands.MapAction(FVoxelGraphCommands::Get().Compile, MakeLambdaDelegate([=]
	{
		Toolkit.Asset->ForeachTerminalGraph_NoInheritance([&](const UVoxelTerminalGraph& TerminalGraph)
		{
			GVoxelGraphTracker->NotifyEdGraphChanged(TerminalGraph.GetEdGraph());
		});
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().CompileAll, MakeLambdaDelegate([=]
	{
		GVoxelGraphEditorInterface->CompileAll();
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().EnableStats, MakeLambdaDelegate([=]
	{
		GVoxelEnableNodeStats = !GVoxelEnableNodeStats;

		if (!GVoxelEnableNodeStats)
		{
			for (IVoxelNodeStatProvider* Provider : GVoxelNodeStatProviders)
			{
				if (!Provider->IsEnabled(*Toolkit.Asset))
				{
					Provider->ClearStats();
				}
			}
		}
	}),
	MakeLambdaDelegate([=]
	{
		return true;
	}),
	MakeLambdaDelegate([=]
	{
		return GVoxelEnableNodeStats;
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().EnableRangeStats, MakeLambdaDelegate([=]
	{
		Toolkit.Asset->bEnableNodeRangeStats = !Toolkit.Asset->bEnableNodeRangeStats;

		if (!Toolkit.Asset->bEnableNodeRangeStats)
		{
			for (IVoxelNodeStatProvider* Provider : GVoxelNodeStatProviders)
			{
				if (!Provider->IsEnabled(*Toolkit.Asset))
				{
					Provider->ClearStats();
				}
			}
		}

		Toolkit.Asset->ForeachTerminalGraph_NoInheritance([&](const UVoxelTerminalGraph& TerminalGraph)
		{
			GVoxelGraphTracker->NotifyEdGraphChanged(TerminalGraph.GetEdGraph());
		});
	}),
	MakeLambdaDelegate([=]
	{
		return true;
	}),
	MakeLambdaDelegate([=]
	{
		return Toolkit.Asset->bEnableNodeRangeStats;
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().FindInGraph, MakeLambdaDelegate([=]
	{
		Toolkit.GetSearchWidget()->FocusForUse({});
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().FindInGraphs, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SVoxelGraphSearch> GlobalSearch = GVoxelGraphSearchManager->OpenGlobalSearch())
		{
			GlobalSearch->FocusForUse({});
		}
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().ReconstructAllNodes, MakeLambdaDelegate([=]
	{
		Toolkit.Asset->ForeachTerminalGraph_NoInheritance([&](const UVoxelTerminalGraph& TerminalGraph)
		{
			for (UEdGraphNode* Node : TerminalGraph.GetEdGraph().Nodes)
			{
				if (!ensure(Node))
				{
					continue;
				}

				Node->ReconstructNode();
			}
		});
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().ToggleDebug, MakeLambdaDelegate([=]
	{
		TVoxelArray<UVoxelGraphNode*> Nodes;
		for (UEdGraphNode* Node : Toolkit.GetSelectedNodes())
		{
			if (UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node))
			{
				Nodes.Add(VoxelNode);
			}
		}

		if (Nodes.Num() != 1)
		{
			return;
		}

		UVoxelGraphNode* Node = Nodes[0];

		if (Node->bEnableDebug)
		{
			Node->bEnableDebug = false;
			GVoxelGraphTracker->NotifyEdGraphNodeChanged(*Node);
			return;
		}

		for (const TObjectPtr<UEdGraphNode>& OtherNode : Node->GetGraph()->Nodes)
		{
			if (UVoxelGraphNode* OtherVoxelNode = Cast<UVoxelGraphNode>(OtherNode.Get()))
			{
				if (OtherVoxelNode->bEnableDebug)
				{
					OtherVoxelNode->bEnableDebug = false;
				}
			}
		}

		Node->bEnableDebug = true;
		GVoxelGraphTracker->NotifyEdGraphNodeChanged(*Node);
	}));

	Commands.MapAction(FVoxelGraphCommands::Get().TogglePreview, MakeLambdaDelegate([=]
	{
		TVoxelArray<UVoxelGraphNode*> Nodes;
		for (UEdGraphNode* Node : Toolkit.GetSelectedNodes())
		{
			if (UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node))
			{
				Nodes.Add(VoxelNode);
			}
		}

		if (Nodes.Num() != 1)
		{
			return;
		}

		UVoxelGraphNode* Node = Nodes[0];

		if (Node->bEnablePreview)
		{
			const FVoxelTransaction Transaction(Node, "Stop previewing");
			Node->bEnablePreview = false;
			return;
		}

		const FVoxelTransaction Transaction(Node, "Start previewing");

		for (const TObjectPtr<UEdGraphNode>& OtherNode : Node->GetGraph()->Nodes)
		{
			if (UVoxelGraphNode* OtherVoxelNode = Cast<UVoxelGraphNode>(OtherNode.Get()))
			{
				if (OtherVoxelNode->bEnablePreview)
				{
					const FVoxelTransaction OtherTransaction(OtherVoxelNode, "Stop previewing");
					OtherVoxelNode->bEnablePreview = false;
				}
			}
		}

		Node->bEnablePreview = true;
	}));

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	Commands.MapAction(FGraphEditorCommands::Get().CreateComment, MakeLambdaDelegate([=]
	{
		const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
		if (!ensure(GraphEditor))
		{
			return;
		}

		UEdGraph* Graph = GraphEditor->GetCurrentGraph();
		if (!ensure(Graph))
		{
			return;
		}

		FVoxelGraphSchemaAction_NewComment CommentAction;
		CommentAction.PerformAction(Graph, nullptr, GraphEditor->GetPasteLocation());
	}));

	Commands.MapAction(FGraphEditorCommands::Get().SplitStructPin, MakeLambdaDelegate([=]
	{
		const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
		if (!ensure(GraphEditor))
		{
			return;
		}

		UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
		if (!ensure(Pin))
		{
			return;
		}

		UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(Pin->GetOwningNode());
		if (!ensure(Node) ||
			!ensure(Node->CanSplitPin(*Pin)))
		{
			return;
		}

		const FVoxelTransaction Transaction(Node, "Split pin");
		Node->SplitPin(*Pin);
	}));

	Commands.MapAction(FGraphEditorCommands::Get().RecombineStructPin, MakeLambdaDelegate([=]
	{
		const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
		if (!ensure(GraphEditor))
		{
			return;
		}

		UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
		if (!ensure(Pin))
		{
			return;
		}

		UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(Pin->GetOwningNode());
		if (!ensure(Node) ||
			!ensure(Node->CanRecombinePin(*Pin)))
		{
			return;
		}

		const FVoxelTransaction Transaction(Node, "Recombine pin");
		Node->RecombinePin(*Pin);
	}));

	Commands.MapAction(FGraphEditorCommands::Get().ResetPinToDefaultValue, MakeLambdaDelegate([=]
	{
		const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
		if (!ensure(GraphEditor))
		{
			return;
		}

		const UEdGraph* Graph = GraphEditor->GetCurrentGraph();
		if (!ensure(Graph))
		{
			return;
		}

		UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
		if (!ensure(Pin))
		{
			return;
		}

		Graph->GetSchema()->ResetPinToAutogeneratedDefaultValue(Pin);
	}),
	MakeLambdaDelegate([=]
	{
		const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
		if (!ensure(GraphEditor))
		{
			return false;
		}

		const UEdGraphPin* Pin = GraphEditor->GetGraphPinForMenu();
		return Pin && !Pin->DoesDefaultValueMatchAutogenerated();
	}));

	Commands.MapAction(FGraphEditorCommands::Get().FindReferences, MakeLambdaDelegate([=]
	{
		const TSet<UEdGraphNode*> SelectedNodes = Toolkit.GetSelectedNodes();
		if (SelectedNodes.Num() != 1)
		{
			return;
		}

		for (const UEdGraphNode* Node : SelectedNodes)
		{
			const UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node);
			if (!VoxelNode)
			{
				continue;
			}

			Toolkit.GetSearchWidget()->FocusForUse(VoxelNode->GetSearchTerms());
			break;
		}
	}));

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	Commands.MapAction(FGenericCommands::Get().Undo, MakeLambdaDelegate([=]
	{
		GEditor->UndoTransaction();
	}));

	Commands.MapAction(FGenericCommands::Get().Redo, MakeLambdaDelegate([=]
	{
		GEditor->RedoTransaction();
	}));

	Commands.MapAction(FGenericCommands::Get().SelectAll, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->SelectAllNodes();
		}
	}));

	Commands.MapAction(FGenericCommands::Get().Delete, MakeLambdaDelegate([=]
	{
		DeleteNodes(Toolkit.GetSelectedNodes().Array());
	}),
	MakeLambdaDelegate([=]
	{
		const TSet<UEdGraphNode*> SelectedNodes = Toolkit.GetSelectedNodes();
		if (SelectedNodes.Num() == 0)
		{
			return false;
		}

		for (const UEdGraphNode* Node : SelectedNodes)
		{
			if (!Node->CanUserDeleteNode())
			{
				return false;
			}
		}
		return true;
	}));

	Commands.MapAction(FGenericCommands::Get().Copy, MakeLambdaDelegate([=]
	{
		TSet<UEdGraphNode*> Nodes;
		{
			for (UEdGraphNode* Node : Toolkit.GetSelectedNodes())
			{
				if (!Node->CanDuplicateNode())
				{
					continue;
				}

				Node->PrepareForCopying();
				Nodes.Add(Node);
			}
		}

		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(ReinterpretCastRef<TSet<UObject*>>(Nodes), ExportedText);
		FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
	}),
	MakeLambdaDelegate([=]
	{
		for (const UEdGraphNode* Node : Toolkit.GetSelectedNodes())
		{
			if (Node->CanDuplicateNode())
			{
				return true;
			}
		}
		return false;
	}));

	Commands.MapAction(FGenericCommands::Get().Cut, MakeLambdaDelegate([=]
	{
		TSet<UEdGraphNode*> Nodes;
		{
			for (UEdGraphNode* Node : Toolkit.GetSelectedNodes())
			{
				if (!Node->CanDuplicateNode())
				{
					continue;
				}

				Node->PrepareForCopying();
				Nodes.Add(Node);
			}
		}

		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(ReinterpretCastRef<TSet<UObject*>>(Nodes), ExportedText);
		FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

		DeleteNodes(Nodes.Array());
	}),
	MakeLambdaDelegate([=]
	{
		const TSet<UEdGraphNode*> SelectedNodes = Toolkit.GetSelectedNodes();
		if (SelectedNodes.Num() == 0)
		{
			return false;
		}

		for (const UEdGraphNode* Node : SelectedNodes)
		{
			if (!Node->CanUserDeleteNode())
			{
				return false;
			}
		}
		for (const UEdGraphNode* Node : SelectedNodes)
		{
			if (Node->CanDuplicateNode())
			{
				return true;
			}
		}
		return false;
	}));

	Commands.MapAction(FGenericCommands::Get().Paste, MakeLambdaDelegate([=]
	{
		FString TextToImport;
		FPlatformApplicationMisc::ClipboardPaste(TextToImport);

		PasteNodes(TextToImport);
	}),
	MakeLambdaDelegate([=]
	{
		const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
		if (!ensure(GraphEditor))
		{
			return false;
		}

		const UEdGraph* Graph = GraphEditor->GetCurrentGraph();
		if (!ensure(Graph))
		{
			return false;
		}

		FString ClipboardContent;
		FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

		return FEdGraphUtilities::CanImportNodesFromText(Graph, ClipboardContent);
	}));

	Commands.MapAction(FGenericCommands::Get().Duplicate, MakeLambdaDelegate([=]
	{
		TSet<UEdGraphNode*> Nodes;
		{
			for (UEdGraphNode* Node : Toolkit.GetSelectedNodes())
			{
				if (!Node->CanDuplicateNode())
				{
					continue;
				}

				Node->PrepareForCopying();
				Nodes.Add(Node);
			}
		}

		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(ReinterpretCastRef<TSet<UObject*>>(Nodes), ExportedText);

		PasteNodes(ExportedText);
	}),
	MakeLambdaDelegate([=]
	{
		for (const UEdGraphNode* Node : Toolkit.GetSelectedNodes())
		{
			if (Node->CanDuplicateNode())
			{
				return true;
			}
		}
		return false;
	}));

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	Commands.MapAction(FGraphEditorCommands::Get().AlignNodesTop, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnAlignTop();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().AlignNodesMiddle, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnAlignMiddle();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().AlignNodesBottom, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnAlignBottom();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().AlignNodesLeft, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnAlignLeft();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().AlignNodesCenter, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnAlignCenter();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().AlignNodesRight, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnAlignRight();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().StraightenConnections, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnStraightenConnections();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().DistributeNodesHorizontally, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnDistributeNodesH();
		}
	}));

	Commands.MapAction(FGraphEditorCommands::Get().DistributeNodesVertically, MakeLambdaDelegate([=]
	{
		if (const TSharedPtr<SGraphEditor> ActiveGraphEditor = Toolkit.GetActiveGraphEditor())
		{
			ActiveGraphEditor->OnDistributeNodesV();
		}
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCommandManager::PasteNodes(const FString& TextToImport) const
{
	const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	PasteNodes(GraphEditor->GetPasteLocation(), TextToImport);
}

void FVoxelGraphCommandManager::PasteNodes(
	const FVector2D& Location,
	const FString& TextToImport) const
{
	const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	UEdGraph* Graph = GraphEditor->GetCurrentGraph();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelTransaction Transaction(Graph, "Paste nodes");

	// Clear the selection set (newly pasted stuff will be selected)
	GraphEditor->ClearSelectionSet();

	// Import the nodes
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(Graph, TextToImport, PastedNodes);

	TSet<UEdGraphNode*> CopyPastedNodes = PastedNodes;
	for (UEdGraphNode* Node : CopyPastedNodes)
	{
		if (UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node))
		{
			if (!VoxelNode->CanPasteVoxelNode(PastedNodes))
			{
				Node->DestroyNode();
				PastedNodes.Remove(Node);
			}
		}
	}

	// Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (const UEdGraphNode* Node : PastedNodes)
	{
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	if (PastedNodes.Num() > 0)
	{
		AvgNodePosition.X /= PastedNodes.Num();
		AvgNodePosition.Y /= PastedNodes.Num();
	}

	for (UEdGraphNode* Node : PastedNodes)
	{
		// Select the newly pasted stuff
		GraphEditor->SetNodeSelection(Node, true);

		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;

		Node->SnapToGrid(SNodePanel::GetSnapGridSize());

		// Give new node a different GUID from the old one
		Node->CreateNewGuid();
	}

	// Post paste for local variables
	for (UEdGraphNode* Node : PastedNodes)
	{
		if (UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Node))
		{
			VoxelNode->bEnableDebug = false;
			VoxelNode->bEnablePreview = false;
			VoxelNode->PostPasteVoxelNode(PastedNodes);
		}
	}

	// Update UI
	GraphEditor->NotifyGraphChanged();
}

void FVoxelGraphCommandManager::DeleteNodes(const TArray<UEdGraphNode*>& InNodes) const
{
	if (InNodes.Num() == 0)
	{
		return;
	}

	// Ensure we're not going to edit the array while iterating it, eg if it's directly the graph node array
	const TArray<UEdGraphNode*> Nodes = InNodes;

	const TSharedPtr<SGraphEditor> GraphEditor = Toolkit.GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	UEdGraph* Graph = GraphEditor->GetCurrentGraph();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelTransaction Transaction(Graph, "Delete nodes");

	for (UEdGraphNode* Node : Nodes)
	{
		if (!ensure(Node) || !Node->CanUserDeleteNode())
		{
			continue;
		}

		GraphEditor->SetNodeSelection(Node, false);

		Node->Modify();
		Graph->GetSchema()->BreakNodeLinks(*Node);
		Node->DestroyNode();
	}
}