// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMembersSchemaAction_Input.h"
#include "SVoxelGraphMembers.h"
#include "VoxelExposedSeed.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphVisuals.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "Nodes/VoxelGraphNode_Input.h"

void FVoxelMembersSchemaAction_Input::OnPaste(
	const FVoxelGraphToolkit& Toolkit,
	UVoxelTerminalGraph& TerminalGraph,
	const FString& Text,
	const FString& Category)
{
	FVoxelGraphInput NewInput;
	if (!FVoxelUtilities::TryImportText(Text, NewInput))
	{
		return;
	}

	NewInput.Category = Category;

	const FGuid NewGuid = FGuid::NewGuid();
	{
		const FVoxelTransaction Transaction(TerminalGraph, "Paste input");
		TerminalGraph.AddInput(NewGuid, NewInput);
	}

	if (TerminalGraph.IsMainTerminalGraph())
	{
		Toolkit.GetSelection().SelectGraphInput(NewGuid);
	}
	else
	{
		Toolkit.GetSelection().SelectFunctionInput(TerminalGraph, NewGuid);
	}
	Toolkit.GetSelection().RequestRename();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UObject* FVoxelMembersSchemaAction_Input::GetOuter() const
{
	return WeakTerminalGraph.Get();
}

void FVoxelMembersSchemaAction_Input::ApplyNewGuids(const TArray<FGuid>& NewGuids) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Reorder inputs");
	TerminalGraph->ReorderInputs(NewGuids);
}

FReply FVoxelMembersSchemaAction_Input::OnDragged(const FPointerEvent& MouseEvent)
{
	return FReply::Handled().BeginDragDrop(FVoxelMembersDragDropAction_Input::New(
		SharedThis(this),
		WeakTerminalGraph,
		MemberGuid));
}

void FVoxelMembersSchemaAction_Input::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	if (!ensure(WeakTerminalGraph.IsValid()) ||
		!WeakTerminalGraph->IsInheritedInput(MemberGuid))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		INVTEXT("Go to definition"),
		INVTEXT("Go to the graph defining this input"),
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
					if (!BaseTerminalGraph->FindInput(MemberGuid))
					{
						break;
					}
					LastTerminalGraph = BaseTerminalGraph;
				}

				FVoxelUtilities::FocusObject(LastTerminalGraph);
			})
		});
}

void FVoxelMembersSchemaAction_Input::OnActionDoubleClick() const
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
		if (const UVoxelGraphNode_Input* InputNode = Cast<UVoxelGraphNode_Input>(Node))
		{
			if (InputNode->Guid == MemberGuid)
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

void FVoxelMembersSchemaAction_Input::OnSelected() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	if (TerminalGraph->IsMainTerminalGraph())
	{
		Toolkit->GetSelection().SelectGraphInput(MemberGuid);
	}
	else
	{
		Toolkit->GetSelection().SelectFunctionInput(*TerminalGraph, MemberGuid);
	}
}

void FVoxelMembersSchemaAction_Input::OnDelete() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(MemberGuid);
	if (!ensure(Input))
	{
		return;
	}

	UVoxelGraph::LoadAllGraphs();

	TArray<UVoxelGraphNode_Input*> InputNodes;
	for (UVoxelTerminalGraph* ChildTerminalGraph : TerminalGraph->GetChildTerminalGraphs_LoadedOnly())
	{
		for (UEdGraphNode* Node : ChildTerminalGraph->GetEdGraph().Nodes)
		{
			if (UVoxelGraphNode_Input* InputNode = Cast<UVoxelGraphNode_Input>(Node))
			{
				if (InputNode->Guid == MemberGuid)
				{
					InputNodes.Add(InputNode);
				}
			}
		}
	}

	EResult DeleteNodes = EResult::No;
	if (InputNodes.Num() > 0)
	{
		DeleteNodes = CreateDeletePopups("Delete input", Input->Name.ToString());

		if (DeleteNodes == EResult::Cancel)
		{
			return;
		}
	}

	{
		const FVoxelTransaction Transaction(TerminalGraph, "Delete input");

		if (DeleteNodes == EResult::Yes)
		{
			for (UVoxelGraphNode_Input* Node : InputNodes)
			{
				Node->GetGraph()->RemoveNode(Node);
			}
		}
		else
		{
			// Reset GUIDs to avoid invalid references
			for (UVoxelGraphNode_Input* Node : InputNodes)
			{
				Node->Guid = {};
			}
		}

		TerminalGraph->RemoveInput(MemberGuid);
	}

	MarkAsDeleted();

	if (const TSharedPtr<SVoxelGraphMembers> Members = GetMembers())
	{
		Members->SelectClosestAction(GetSectionID(), MemberGuid);
	}
}

void FVoxelMembersSchemaAction_Input::OnDuplicate() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(MemberGuid);
	if (!ensure(Input))
	{
		return;
	}

	const FGuid NewGuid = FGuid::NewGuid();

	{
		const FVoxelTransaction Transaction(TerminalGraph, "Duplicate input");
		TerminalGraph->AddInput(NewGuid, MakeCopy(*Input));
	}

	if (TerminalGraph->IsMainTerminalGraph())
	{
		Toolkit->GetSelection().SelectGraphInput(NewGuid);
	}
	else
	{
		Toolkit->GetSelection().SelectFunctionInput(*TerminalGraph, NewGuid);
	}
	Toolkit->GetSelection().RequestRename();
}

bool FVoxelMembersSchemaAction_Input::OnCopy(FString& OutText) const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(MemberGuid);
	if (!ensure(Input))
	{
		return false;
	}

	const FVoxelGraphInput Defaults;

	FVoxelGraphInput::StaticStruct()->ExportText(OutText, Input, &Defaults, nullptr, 0, nullptr);
	return true;
}

FString FVoxelMembersSchemaAction_Input::GetName() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return {};
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(MemberGuid);
	if (!ensure(Input))
	{
		return {};
	}

	return Input->Name.ToString();
}

void FVoxelMembersSchemaAction_Input::SetName(const FString& Name) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Rename input");

	TerminalGraph->UpdateInput(MemberGuid, [&](FVoxelGraphInput& Input)
	{
		Input.Name = *Name;
	});
}

void FVoxelMembersSchemaAction_Input::SetCategory(const FString& NewCategory) const
{
	if (bIsInherited)
	{
		return;
	}

	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Set input category");

	TerminalGraph->UpdateInput(MemberGuid, [&](FVoxelGraphInput& Input)
	{
		Input.Category = *NewCategory;
	});
}

FString FVoxelMembersSchemaAction_Input::GetSearchString() const
{
	return MemberGuid.ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType FVoxelMembersSchemaAction_Input::GetPinType() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return {};
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(MemberGuid);
	if (!ensure(Input))
	{
		return {};
	}

	return Input->Type;
}

void FVoxelMembersSchemaAction_Input::SetPinType(const FVoxelPinType& NewPinType) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Change input type");

	TerminalGraph->UpdateInput(MemberGuid, [&](FVoxelGraphInput& Input)
	{
		Input.Type = NewPinType;
		Input.Fixup();

		if (Input.DefaultValue.Is<FVoxelExposedSeed>())
		{
			Input.DefaultValue.Get<FVoxelExposedSeed>().Randomize();
		}
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Input::HoverTargetChanged()
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

	if (!IsCompatibleEdGraph(*EdGraph))
	{
		SetFeedbackMessageError("Cannot use in a different graph");
		return;
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(InputGuid);
	if (!ensure(Input))
	{
		return;
	}

	if (const UEdGraphPin* Pin = GetHoveredPin())
	{
		if (!CheckPin(Pin, { EGPD_Input }))
		{
			return;
		}

		if (!Input->Type.CanBeCastedTo_Schema(Pin->PinType))
		{
			SetFeedbackMessageError("The type of '" + Input->Name.ToString() + "' is not compatible with " + Pin->PinName.ToString());
			return;
		}

		SetFeedbackMessageOK("Make " + Pin->PinName.ToString() + " = " + Input->Name.ToString() + "");
		return;
	}

	if (Cast<UVoxelGraphNode_Input>(GetHoveredNode()))
	{
		SetFeedbackMessageOK("Change node input");
		return;
	}

	FVoxelMembersDragDropAction_Base::HoverTargetChanged();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FReply FVoxelMembersDragDropAction_Input::DroppedOnPin(const FVector2D ScreenPosition, const FVector2D GraphPosition)
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

	if (!IsCompatibleEdGraph(*EdGraph))
	{
		return FReply::Unhandled();
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(InputGuid);
	if (!ensure(Input))
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
		Pin->Direction != EGPD_Input ||
		!Input->Type.CanBeCastedTo_Schema(Pin->PinType))
	{
		return FReply::Unhandled();
	}

	if (TerminalGraph->IsMainTerminalGraph())
	{
		FVoxelGraphSchemaAction_NewGraphInputUsage Action;
		Action.Guid = InputGuid;
		Action.PerformAction(Pin->GetOwningNode()->GetGraph(), Pin, GraphPosition, true);
	}
	else
	{
		FVoxelGraphSchemaAction_NewFunctionInputUsage Action;
		Action.Guid = InputGuid;
		Action.PerformAction(Pin->GetOwningNode()->GetGraph(), Pin, GraphPosition, true);
	}

	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_Input::DroppedOnNode(const FVector2D ScreenPosition, const FVector2D GraphPosition)
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

	if (!IsCompatibleEdGraph(*EdGraph))
	{
		return FReply::Unhandled();
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(InputGuid);
	if (!ensure(Input))
	{
		return FReply::Unhandled();
	}

	UEdGraphNode* Node = GetHoveredNode();
	if (!ensure(Node))
	{
		return FReply::Unhandled();
	}

	UVoxelGraphNode_Input* InputNode = Cast<UVoxelGraphNode_Input>(Node);
	if (!InputNode)
	{
		return FReply::Unhandled();
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Replace input");
	InputNode->Guid = InputGuid;
	InputNode->bIsGraphInput = TerminalGraph->IsMainTerminalGraph();
	InputNode->CachedInput = *Input;
	InputNode->ReconstructNode();

	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_Input::DroppedOnPanel(
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

	if (!IsCompatibleEdGraph(EdGraph))
	{
		return FReply::Unhandled();
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(InputGuid);
	if (!ensure(Input))
	{
		return FReply::Unhandled();
	}

	if (TerminalGraph->IsMainTerminalGraph())
	{
		FVoxelGraphSchemaAction_NewGraphInputUsage Action;
		Action.Guid = InputGuid;
		Action.PerformAction(&EdGraph, nullptr, GraphPosition, true);
	}
	else
	{
		FVoxelGraphSchemaAction_NewFunctionInputUsage Action;
		Action.Guid = InputGuid;
		Action.PerformAction(&EdGraph, nullptr, GraphPosition, true);
	}

	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Input::GetDefaultStatusSymbol(
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

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(InputGuid);
	if (!ensure(Input))
	{
		return;
	}

	PrimaryBrushOut = FVoxelGraphVisuals::GetPinIcon(Input->Type).GetIcon();
	IconColorOut = FVoxelGraphVisuals::GetPinColor(Input->Type);
}

bool FVoxelMembersDragDropAction_Input::IsCompatibleEdGraph(const UEdGraph& EdGraph) const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	if (EdGraph.GetTypedOuter<UVoxelGraph>() != &TerminalGraph->GetGraph())
	{
		return false;
	}

	if (TerminalGraph->IsMainTerminalGraph())
	{
		// Graph input
		return true;
	}

	return EdGraph.GetTypedOuter<UVoxelTerminalGraph>() == TerminalGraph;
}