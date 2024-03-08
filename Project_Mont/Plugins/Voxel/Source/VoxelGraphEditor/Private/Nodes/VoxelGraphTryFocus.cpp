// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelGraphToolkit.h"
#include "VoxelTerminalGraph.h"

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterTryFocus)
{
	GVoxelTryFocusObjectFunctions.Add([](const UObject& Object)
	{
		const UVoxelTerminalGraph* TerminalGraph = Cast<UVoxelTerminalGraph>(&Object);
		if (!TerminalGraph)
		{
			if (const UEdGraph* EdGraph = Cast<UEdGraph>(&Object))
			{
				TerminalGraph = EdGraph->GetTypedOuter<UVoxelTerminalGraph>();
			}
		}
		if (!TerminalGraph)
		{
			return false;
		}

		const FVoxelGraphToolkit* Toolkit = FVoxelToolkit::OpenToolkit<FVoxelGraphToolkit>(TerminalGraph->GetGraph());
		if (!ensure(Toolkit))
		{
			return false;
		}

		(void)Toolkit->OpenGraphAndBringToFront(TerminalGraph->GetEdGraph(), true);

		Toolkit->GetSelection().SelectTerminalGraph(*TerminalGraph);
		return true;
	});

	GVoxelTryFocusObjectFunctions.Add([](const UObject& Object)
	{
		const UEdGraphNode* EdGraphNode = Cast<UEdGraphNode>(&Object);
		if (!EdGraphNode)
		{
			return false;
		}

		const UVoxelGraph* Graph = EdGraphNode->GetTypedOuter<UVoxelGraph>();
		if (!Graph)
		{
			return false;
		}

		const UEdGraph* EdGraph = EdGraphNode->GetGraph();
		if (!ensure(EdGraph))
		{
			return false;
		}

		const FVoxelGraphToolkit* Toolkit = FVoxelToolkit::OpenToolkit<FVoxelGraphToolkit>(*Graph);
		if (!ensure(Toolkit))
		{
			return false;
		}

		const TSharedPtr<SGraphEditor> GraphEditor = Toolkit->OpenGraphAndBringToFront(*EdGraph, true);
		if (!ensure(GraphEditor))
		{
			return false;
		}

		GraphEditor->ClearSelectionSet();
		GraphEditor->SetNodeSelection(ConstCast(EdGraphNode), true);
		GraphEditor->ZoomToFit(true);
		return true;
	});
}