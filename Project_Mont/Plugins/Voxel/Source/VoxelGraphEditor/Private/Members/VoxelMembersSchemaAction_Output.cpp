// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMembersSchemaAction_Output.h"
#include "SVoxelGraphMembers.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphVisuals.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "Nodes/VoxelGraphNode_Output.h"

void FVoxelMembersSchemaAction_Output::OnPaste(
	const FVoxelGraphToolkit& Toolkit,
	UVoxelTerminalGraph& TerminalGraph,
	const FString& Text,
	const FString& Category)
{
	FVoxelGraphOutput NewOutput;
	if (!FVoxelUtilities::TryImportText(Text, NewOutput))
	{
		return;
	}

	NewOutput.Category = Category;

	const FGuid NewGuid = FGuid::NewGuid();
	{
		const FVoxelTransaction Transaction(TerminalGraph, "Paste output");
		TerminalGraph.AddOutput(NewGuid, NewOutput);
	}

	Toolkit.GetSelection().SelectOutput(TerminalGraph, NewGuid);
	Toolkit.GetSelection().RequestRename();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UObject* FVoxelMembersSchemaAction_Output::GetOuter() const
{
	return WeakTerminalGraph.Get();
}

void FVoxelMembersSchemaAction_Output::ApplyNewGuids(const TArray<FGuid>& NewGuids) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Reorder outputs");
	TerminalGraph->ReorderOutputs(NewGuids);
}

FReply FVoxelMembersSchemaAction_Output::OnDragged(const FPointerEvent& MouseEvent)
{
	return FReply::Handled().BeginDragDrop(FVoxelMembersDragDropAction_Output::New(
		SharedThis(this),
		WeakTerminalGraph,
		MemberGuid));
}

void FVoxelMembersSchemaAction_Output::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	if (!ensure(WeakTerminalGraph.IsValid()) ||
		!WeakTerminalGraph->IsInheritedOutput(MemberGuid))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		INVTEXT("Go to definition"),
		INVTEXT("Go to the graph defining this output"),
		{},
		FUIAction
		{
			MakeLambdaDelegate([=]
			{
				const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
				if (!ensure(TerminalGraph))
				{
					return;
				}

				const UVoxelTerminalGraph* LastTerminalGraph = TerminalGraph;
				for (const UVoxelTerminalGraph* BaseTerminalGraph : TerminalGraph->GetBaseTerminalGraphs())
				{
					if (!BaseTerminalGraph->FindOutput(MemberGuid))
					{
						break;
					}
					LastTerminalGraph = BaseTerminalGraph;
				}

				FVoxelUtilities::FocusObject(LastTerminalGraph);
			})
		});
}

void FVoxelMembersSchemaAction_Output::OnActionDoubleClick() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	const TSharedPtr<SGraphEditor> GraphEditor = Toolkit->GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return;
	}

	GraphEditor->ClearSelectionSet();

	for (UEdGraphNode* Node : GraphEditor->GetCurrentGraph()->Nodes)
	{
		if (const UVoxelGraphNode_Output* OutputNode = Cast<UVoxelGraphNode_Output>(Node))
		{
			if (OutputNode->Guid == MemberGuid)
			{
				GraphEditor->SetNodeSelection(Node, true);
			}
		}
	}

	if (GraphEditor->GetSelectedNodes().Num() > 0)
	{
		GraphEditor->ZoomToFit(true);
	}
}

void FVoxelMembersSchemaAction_Output::OnSelected() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	Toolkit->GetSelection().SelectOutput(*TerminalGraph, MemberGuid);
}

void FVoxelMembersSchemaAction_Output::OnDelete() const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(MemberGuid);
	if (!ensure(Output))
	{
		return;
	}

	UVoxelGraph::LoadAllGraphs();

	TArray<UVoxelGraphNode_Output*> OutputNodes;
	for (UVoxelTerminalGraph* ChildTerminalGraph : TerminalGraph->GetChildTerminalGraphs_LoadedOnly())
	{
		for (UEdGraphNode* Node : ChildTerminalGraph->GetEdGraph().Nodes)
		{
			if (UVoxelGraphNode_Output* OutputNode = Cast<UVoxelGraphNode_Output>(Node))
			{
				if (OutputNode->Guid == MemberGuid)
				{
					OutputNodes.Add(OutputNode);
				}
			}
		}
	}

	EResult DeleteNodes = EResult::No;
	if (OutputNodes.Num() > 0)
	{
		DeleteNodes = CreateDeletePopups("Delete output", Output->Name.ToString());

		if (DeleteNodes == EResult::Cancel)
		{
			return;
		}
	}

	{
		const FVoxelTransaction Transaction(TerminalGraph, "Delete output");

		if (DeleteNodes == EResult::Yes)
		{
			for (UVoxelGraphNode_Output* Node : OutputNodes)
			{
				Node->GetGraph()->RemoveNode(Node);
			}
		}
		else
		{
			// Reset GUIDs to avoid invalid references
			for (UVoxelGraphNode_Output* Node : OutputNodes)
			{
				Node->Guid = {};
			}
		}

		TerminalGraph->RemoveOutput(MemberGuid);
	}

	MarkAsDeleted();

	if (const TSharedPtr<SVoxelGraphMembers> Members = GetMembers())
	{
		Members->SelectClosestAction(GetSectionID(), MemberGuid);
	}
}

void FVoxelMembersSchemaAction_Output::OnDuplicate() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(MemberGuid);
	if (!ensure(Output))
	{
		return;
	}

	const FGuid NewGuid = FGuid::NewGuid();

	{
		const FVoxelTransaction Transaction(TerminalGraph, "Duplicate output");
		TerminalGraph->AddOutput(NewGuid, MakeCopy(*Output));
	}

	Toolkit->GetSelection().SelectOutput(*TerminalGraph, NewGuid);
	Toolkit->GetSelection().RequestRename();
}

bool FVoxelMembersSchemaAction_Output::OnCopy(FString& OutText) const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(MemberGuid);
	if (!ensure(Output))
	{
		return false;
	}

	const FVoxelGraphOutput Defaults;

	FVoxelGraphOutput::StaticStruct()->ExportText(OutText, Output, &Defaults, nullptr, 0, nullptr);
	return true;
}

FString FVoxelMembersSchemaAction_Output::GetName() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return {};
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(MemberGuid);
	if (!ensureVoxelSlow(Output))
	{
		return {};
	}

	return Output->Name.ToString();
}

void FVoxelMembersSchemaAction_Output::SetName(const FString& Name) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	{
		const FVoxelTransaction Transaction(TerminalGraph, "Rename output");

		TerminalGraph->UpdateOutput(MemberGuid, [&](FVoxelGraphOutput& Output)
		{
			Output.Name = *Name;
		});
	}
}

void FVoxelMembersSchemaAction_Output::SetCategory(const FString& NewCategory) const
{
	if (bIsInherited)
	{
		return;
	}

	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Set output category");

	TerminalGraph->UpdateOutput(MemberGuid, [&](FVoxelGraphOutput& Output)
	{
		Output.Category = *NewCategory;
	});
}

FString FVoxelMembersSchemaAction_Output::GetSearchString() const
{
	return MemberGuid.ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType FVoxelMembersSchemaAction_Output::GetPinType() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return {};
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(MemberGuid);
	if (!ensure(Output))
	{
		return {};
	}

	return Output->Type;
}

void FVoxelMembersSchemaAction_Output::SetPinType(const FVoxelPinType& NewPinType) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Change output type");

	TerminalGraph->UpdateOutput(MemberGuid, [&](FVoxelGraphOutput& Output)
	{
		Output.Type = NewPinType;
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Output::HoverTargetChanged()
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const UEdGraph* EdGraph = GetHoveredGraph();
	if (!EdGraph)
	{
		FVoxelMembersDragDropAction_Base::HoverTargetChanged();
		return;
	}

	if (EdGraph->GetTypedOuter<UVoxelTerminalGraph>() != TerminalGraph)
	{
		SetFeedbackMessageError("Cannot use in a different graph");
		return;
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(OutputGuid);
	if (!ensure(Output))
	{
		return;
	}

	if (const UEdGraphPin* Pin = GetHoveredPin())
	{
		if (!CheckPin(Pin, { EGPD_Output }))
		{
			return;
		}

		if (!FVoxelPinType(Pin->PinType).CanBeCastedTo_Schema(Output->Type))
		{
			SetFeedbackMessageError("The type of '" + Output->Name.ToString() + "' is not compatible with " + Pin->PinName.ToString());
			return;
		}

		SetFeedbackMessageOK("Make " + Output->Name.ToString() + " = " + Pin->PinName.ToString() + "");
		return;
	}

	if (Cast<UVoxelGraphNode_Output>(GetHoveredNode()))
	{
		SetFeedbackMessageOK("Change node output");
		return;
	}

	FVoxelMembersDragDropAction_Base::HoverTargetChanged();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FReply FVoxelMembersDragDropAction_Output::DroppedOnPin(const FVector2D ScreenPosition, const FVector2D GraphPosition)
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return FReply::Unhandled();
	}

	const UEdGraph* EdGraph = GetHoveredGraph();
	if (!EdGraph)
	{
		return FReply::Unhandled();
	}

	if (EdGraph->GetTypedOuter<UVoxelTerminalGraph>() != TerminalGraph)
	{
		return FReply::Unhandled();
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(OutputGuid);
	if (!ensure(Output))
	{
		return FReply::Unhandled();
	}

	UEdGraphPin* Pin = GetHoveredPin();
	if (!ensure(Pin))
	{
		return FReply::Unhandled();
	}

	if (Pin->bOrphanedPin ||
		Pin->bNotConnectable ||
		Pin->Direction != EGPD_Output ||
		!FVoxelPinType(Pin->PinType).CanBeCastedTo_Schema(Output->Type))
	{
		return FReply::Unhandled();
	}

	FVoxelGraphSchemaAction_NewOutputUsage Action;
	Action.Guid = OutputGuid;
	Action.PerformAction(Pin->GetOwningNode()->GetGraph(), Pin, GraphPosition, true);

	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_Output::DroppedOnNode(const FVector2D ScreenPosition, const FVector2D GraphPosition)
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return FReply::Unhandled();
	}

	const UEdGraph* EdGraph = GetHoveredGraph();
	if (!EdGraph)
	{
		return FReply::Unhandled();
	}

	if (EdGraph->GetTypedOuter<UVoxelTerminalGraph>() != TerminalGraph)
	{
		return FReply::Unhandled();
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(OutputGuid);
	if (!ensure(Output))
	{
		return FReply::Unhandled();
	}

	UEdGraphNode* Node = GetHoveredNode();
	if (!ensure(Node))
	{
		return FReply::Unhandled();
	}

	UVoxelGraphNode_Output* OutputNode = Cast<UVoxelGraphNode_Output>(Node);
	if (!OutputNode)
	{
		return FReply::Unhandled();
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Replace output");
	OutputNode->Guid = OutputGuid;
	OutputNode->CachedOutput = *Output;
	OutputNode->ReconstructNode();

	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_Output::DroppedOnPanel(
	const TSharedRef<SWidget>& Panel,
	const FVector2D ScreenPosition,
	const FVector2D GraphPosition,
	UEdGraph& EdGraph)
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return FReply::Unhandled();
	}

	if (EdGraph.GetTypedOuter<UVoxelTerminalGraph>() != TerminalGraph)
	{
		return FReply::Unhandled();
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(OutputGuid);
	if (!ensure(Output))
	{
		return FReply::Unhandled();
	}

	FVoxelGraphSchemaAction_NewOutputUsage Action;
	Action.Guid = OutputGuid;
	Action.PerformAction(&EdGraph, nullptr, GraphPosition, true);

	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Output::GetDefaultStatusSymbol(
	const FSlateBrush*& PrimaryBrushOut,
	FSlateColor& IconColorOut,
	FSlateBrush const*& SecondaryBrushOut,
	FSlateColor& SecondaryColorOut) const
{
	FGraphSchemaActionDragDropAction::GetDefaultStatusSymbol(PrimaryBrushOut, IconColorOut, SecondaryBrushOut, SecondaryColorOut);

	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(OutputGuid);
	if (!ensure(Output))
	{
		return;
	}

	PrimaryBrushOut = FVoxelGraphVisuals::GetPinIcon(Output->Type).GetIcon();
	IconColorOut = FVoxelGraphVisuals::GetPinColor(Output->Type);
}