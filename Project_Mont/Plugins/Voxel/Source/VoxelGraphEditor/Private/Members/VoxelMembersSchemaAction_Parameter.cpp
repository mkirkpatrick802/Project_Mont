// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMembersSchemaAction_Parameter.h"
#include "SVoxelGraphMembers.h"
#include "VoxelParameter.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphVisuals.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "Nodes/VoxelGraphNode_Parameter.h"

void FVoxelMembersSchemaAction_Parameter::OnPaste(
	const FVoxelGraphToolkit& Toolkit,
	const FString& Text,
	const FString& Category)
{
	FVoxelParameter NewParameter;
	if (!FVoxelUtilities::TryImportText(Text, NewParameter))
	{
		return;
	}

	NewParameter.Category = Category;

	const FGuid NewGuid = FGuid::NewGuid();
	{
		const FVoxelTransaction Transaction(Toolkit.Asset, "Paste parameter");
		Toolkit.Asset->AddParameter(NewGuid, NewParameter);
	}

	Toolkit.GetSelection().SelectParameter(NewGuid);
	Toolkit.GetSelection().RequestRename();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UObject* FVoxelMembersSchemaAction_Parameter::GetOuter() const
{
	return WeakGraph.Get();
}

void FVoxelMembersSchemaAction_Parameter::ApplyNewGuids(const TArray<FGuid>& NewGuids) const
{
	UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelTransaction Transaction(Graph, "Reorder parameters");
	Graph->ReorderParameters(NewGuids);
}

FReply FVoxelMembersSchemaAction_Parameter::OnDragged(const FPointerEvent& MouseEvent)
{
	return FReply::Handled().BeginDragDrop(FVoxelMembersDragDropAction_Parameter::New(
		SharedThis(this),
		WeakGraph.Get(),
		MemberGuid));
}

void FVoxelMembersSchemaAction_Parameter::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	if (!ensure(WeakGraph.IsValid()) ||
		!WeakGraph->IsInheritedParameter(MemberGuid))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		INVTEXT("Go to definition"),
		INVTEXT("Go to the graph defining this parameter"),
		{},
		FUIAction
		{
			MakeLambdaDelegate([=]
			{
				const UVoxelGraph* Graph = WeakGraph.Get();
				if (!ensure(Graph))
				{
					return;
				}

				const UVoxelGraph* LastGraph = Graph;
				for (const UVoxelGraph* BaseGraph : Graph->GetBaseGraphs())
				{
					if (!BaseGraph->FindParameter(MemberGuid))
					{
						break;
					}
					LastGraph = BaseGraph;
				}

				FVoxelUtilities::FocusObject(LastGraph);
			})
		});
}

void FVoxelMembersSchemaAction_Parameter::OnActionDoubleClick() const
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
		if (const UVoxelGraphNode_Parameter* ParameterNode = Cast<UVoxelGraphNode_Parameter>(Node))
		{
			if (ParameterNode->Guid == MemberGuid)
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

void FVoxelMembersSchemaAction_Parameter::OnSelected() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	Toolkit->GetSelection().SelectParameter(MemberGuid);
}

void FVoxelMembersSchemaAction_Parameter::OnDelete() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(MemberGuid);
	if (!ensure(Parameter))
	{
		return;
	}

	UVoxelGraph::LoadAllGraphs();

	TArray<UVoxelGraphNode_Parameter*> ParameterNodes;
	for (UVoxelGraph* ChildGraph : Graph->GetChildGraphs_LoadedOnly())
	{
		ChildGraph->ForeachTerminalGraph_NoInheritance([&](const UVoxelTerminalGraph& TerminalGraph)
		{
			for (UEdGraphNode* Node : TerminalGraph.GetEdGraph().Nodes)
			{
				if (UVoxelGraphNode_Parameter* ParameterNode = Cast<UVoxelGraphNode_Parameter>(Node))
				{
					if (ParameterNode->Guid == MemberGuid)
					{
						ParameterNodes.Add(ParameterNode);
					}
				}
			}
		});
	}

	EResult DeleteNodes = EResult::No;
	if (ParameterNodes.Num() > 0)
	{
		DeleteNodes = CreateDeletePopups("Delete parameter", Parameter->Name.ToString());

		if (DeleteNodes == EResult::Cancel)
		{
			return;
		}
	}

	{
		const FVoxelTransaction Transaction(Graph, "Delete parameter");

		if (DeleteNodes == EResult::Yes)
		{
			for (UVoxelGraphNode_Parameter* Node : ParameterNodes)
			{
				Node->GetGraph()->RemoveNode(Node);
			}
		}
		else
		{
			// Reset GUIDs to avoid invalid references
			for (UVoxelGraphNode_Parameter* Node : ParameterNodes)
			{
				Node->Guid = {};
			}
		}

		Graph->RemoveParameter(MemberGuid);
	}

	MarkAsDeleted();

	if (const TSharedPtr<SVoxelGraphMembers> Members = GetMembers())
	{
		Members->SelectClosestAction(GetSectionID(), MemberGuid);
	}
}

void FVoxelMembersSchemaAction_Parameter::OnDuplicate() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(MemberGuid);
	if (!ensure(Parameter))
	{
		return;
	}

	const FGuid NewGuid = FGuid::NewGuid();

	{
		const FVoxelTransaction Transaction(Graph, "Duplicate parameter");
		Graph->AddParameter(NewGuid, MakeCopy(*Parameter));
	}

	Toolkit->GetSelection().SelectParameter(NewGuid);
	Toolkit->GetSelection().RequestRename();
}

bool FVoxelMembersSchemaAction_Parameter::OnCopy(FString& OutText) const
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return false;
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(MemberGuid);
	if (!ensure(Parameter))
	{
		return false;
	}

	const FVoxelParameter Defaults;

	FVoxelParameter::StaticStruct()->ExportText(OutText, Parameter, &Defaults, nullptr, 0, nullptr);
	return true;
}

FString FVoxelMembersSchemaAction_Parameter::GetName() const
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return {};
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(MemberGuid);
	if (!ensure(Parameter))
	{
		return {};
	}

	return Parameter->Name.ToString();
}

void FVoxelMembersSchemaAction_Parameter::SetName(const FString& Name) const
{
	UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelTransaction Transaction(Graph, "Rename parameter");

	Graph->UpdateParameter(MemberGuid, [&](FVoxelParameter& Parameter)
	{
		Parameter.Name = *Name;
	});
}

void FVoxelMembersSchemaAction_Parameter::SetCategory(const FString& NewCategory) const
{
	if (bIsInherited)
	{
		return;
	}

	UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelTransaction Transaction(Graph, "Set parameter category");

	Graph->UpdateParameter(MemberGuid, [&](FVoxelParameter& Parameter)
	{
		Parameter.Category = *NewCategory;
	});
}

FString FVoxelMembersSchemaAction_Parameter::GetSearchString() const
{
	return MemberGuid.ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType FVoxelMembersSchemaAction_Parameter::GetPinType() const
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return {};
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(MemberGuid);
	if (!ensure(Parameter))
	{
		return {};
	}

	return Parameter->Type;
}

void FVoxelMembersSchemaAction_Parameter::SetPinType(const FVoxelPinType& NewPinType) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelTransaction Transaction(Graph, "Change parameter type");

	Graph->UpdateParameter(MemberGuid, [&](FVoxelParameter& Parameter)
	{
		Parameter.Type = NewPinType;
		Parameter.Fixup();
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Parameter::HoverTargetChanged()
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const UEdGraph* EdGraph = GetHoveredGraph();
	if (!EdGraph)
	{
		FVoxelMembersDragDropAction_Base::HoverTargetChanged();
		return;
	}

	if (EdGraph->GetTypedOuter<UVoxelGraph>() != Graph)
	{
		SetFeedbackMessageError("Cannot use in a different graph");
		return;
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(ParameterGuid);
	if (!ensure(Parameter))
	{
		return;
	}

	if (const UEdGraphPin* Pin = GetHoveredPin())
	{
		if (!CheckPin(Pin, { EGPD_Input }))
		{
			return;
		}

		if (!Parameter->Type.CanBeCastedTo_Schema(Pin->PinType))
		{
			SetFeedbackMessageError("The type of '" + Parameter->Name.ToString() + "' is not compatible with " + Pin->PinName.ToString());
			return;
		}

		SetFeedbackMessageOK("Make " + Pin->PinName.ToString() + " = " + Parameter->Name.ToString() + "");
		return;
	}

	if (Cast<UVoxelGraphNode_Parameter>(GetHoveredNode()))
	{
		SetFeedbackMessageOK("Change node parameter");
		return;
	}

	FVoxelMembersDragDropAction_Base::HoverTargetChanged();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FReply FVoxelMembersDragDropAction_Parameter::DroppedOnPin(const FVector2D ScreenPosition, const FVector2D GraphPosition)
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return FReply::Unhandled();
	}

	const UEdGraph* EdGraph = GetHoveredGraph();
	if (!EdGraph)
	{
		return FReply::Unhandled();
	}

	if (EdGraph->GetTypedOuter<UVoxelGraph>() != Graph)
	{
		return FReply::Unhandled();
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(ParameterGuid);
	if (!ensure(Parameter))
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
		!Parameter->Type.CanBeCastedTo_Schema(Pin->PinType))
	{
		return FReply::Unhandled();
	}

	FVoxelGraphSchemaAction_NewParameterUsage Action;
	Action.Guid = ParameterGuid;
	Action.PerformAction(Pin->GetOwningNode()->GetGraph(), Pin, GraphPosition, true);

	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_Parameter::DroppedOnNode(const FVector2D ScreenPosition, const FVector2D GraphPosition)
{
	UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return FReply::Unhandled();
	}

	const UEdGraph* EdGraph = GetHoveredGraph();
	if (!EdGraph)
	{
		return FReply::Unhandled();
	}

	if (EdGraph->GetTypedOuter<UVoxelGraph>() != Graph)
	{
		return FReply::Unhandled();
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(ParameterGuid);
	if (!ensure(Parameter))
	{
		return FReply::Unhandled();
	}

	UEdGraphNode* Node = GetHoveredNode();
	if (!ensure(Node))
	{
		return FReply::Unhandled();
	}

	UVoxelGraphNode_Parameter* ParameterNode = Cast<UVoxelGraphNode_Parameter>(Node);
	if (!ParameterNode)
	{
		return FReply::Unhandled();
	}

	const FVoxelTransaction Transaction(Graph, "Replace parameter");
	ParameterNode->Guid = ParameterGuid;
	ParameterNode->CachedParameter = *Parameter;
	ParameterNode->ReconstructNode();

	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_Parameter::DroppedOnPanel(
	const TSharedRef<SWidget>& Panel,
	const FVector2D ScreenPosition,
	const FVector2D GraphPosition,
	UEdGraph& EdGraph)
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return FReply::Unhandled();
	}

	if (EdGraph.GetTypedOuter<UVoxelGraph>() != Graph)
	{
		return FReply::Unhandled();
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(ParameterGuid);
	if (!ensure(Parameter))
	{
		return FReply::Unhandled();
	}

	FVoxelGraphSchemaAction_NewParameterUsage Action;
	Action.Guid = ParameterGuid;
	Action.PerformAction(&EdGraph, nullptr, GraphPosition, true);

	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Parameter::GetDefaultStatusSymbol(
	const FSlateBrush*& PrimaryBrushOut,
	FSlateColor& IconColorOut,
	FSlateBrush const*& SecondaryBrushOut,
	FSlateColor& SecondaryColorOut) const
{
	FGraphSchemaActionDragDropAction::GetDefaultStatusSymbol(PrimaryBrushOut, IconColorOut, SecondaryBrushOut, SecondaryColorOut);

	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(ParameterGuid);
	if (!ensure(Parameter))
	{
		return;
	}

	PrimaryBrushOut = FVoxelGraphVisuals::GetPinIcon(Parameter->Type).GetIcon();
	IconColorOut = FVoxelGraphVisuals::GetPinColor(Parameter->Type);
}