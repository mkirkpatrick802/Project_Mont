// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMembersSchemaAction_LocalVariable.h"
#include "SVoxelGraphMembers.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphVisuals.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "Nodes/VoxelGraphNode_LocalVariable.h"

void FVoxelMembersSchemaAction_LocalVariable::OnPaste(
	const FVoxelGraphToolkit& Toolkit,
	UVoxelTerminalGraph& TerminalGraph,
	const FString& Text,
	const FString& Category)
{
	FVoxelGraphLocalVariable NewLocalVariable;
	if (!FVoxelUtilities::TryImportText(Text, NewLocalVariable))
	{
		return;
	}

	NewLocalVariable.Category = Category;

	const FGuid NewGuid = FGuid::NewGuid();
	{
		const FVoxelTransaction Transaction(TerminalGraph, "Paste local variable");
		TerminalGraph.AddLocalVariable(NewGuid, NewLocalVariable);
	}

	Toolkit.GetSelection().SelectLocalVariable(TerminalGraph, NewGuid);
	Toolkit.GetSelection().RequestRename();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UObject* FVoxelMembersSchemaAction_LocalVariable::GetOuter() const
{
	return WeakTerminalGraph.Get();
}

void FVoxelMembersSchemaAction_LocalVariable::ApplyNewGuids(const TArray<FGuid>& NewGuids) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Reorder local variables");
	TerminalGraph->ReorderLocalVariables(NewGuids);
}

FReply FVoxelMembersSchemaAction_LocalVariable::OnDragged(const FPointerEvent& MouseEvent)
{
	return FReply::Handled().BeginDragDrop(FVoxelMembersDragDropAction_LocalVariable::New(
		SharedThis(this),
		WeakTerminalGraph,
		MemberGuid,
		MouseEvent));
}

void FVoxelMembersSchemaAction_LocalVariable::OnActionDoubleClick() const
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
		if (const UVoxelGraphNode_LocalVariable* LocalVariableNode = Cast<UVoxelGraphNode_LocalVariable>(Node))
		{
			if (LocalVariableNode->Guid == MemberGuid)
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

void FVoxelMembersSchemaAction_LocalVariable::OnSelected() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	Toolkit->GetSelection().SelectLocalVariable(*TerminalGraph, MemberGuid);
}

void FVoxelMembersSchemaAction_LocalVariable::OnDelete() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(MemberGuid);
	if (!ensure(LocalVariable))
	{
		return;
	}

	TArray<UVoxelGraphNode_LocalVariable*> LocalVariableNodes;
	for (UEdGraphNode* Node : TerminalGraph->GetEdGraph().Nodes)
	{
		if (UVoxelGraphNode_LocalVariable* LocalVariableNode = Cast<UVoxelGraphNode_LocalVariable>(Node))
		{
			if (LocalVariableNode->Guid == MemberGuid)
			{
				LocalVariableNodes.Add(LocalVariableNode);
			}
		}
	}

	EResult DeleteNodes = EResult::No;
	if (LocalVariableNodes.Num() > 0)
	{
		DeleteNodes = CreateDeletePopups("Delete local variable", LocalVariable->Name.ToString());

		if (DeleteNodes == EResult::Cancel)
		{
			return;
		}
	}

	{
		const FVoxelTransaction Transaction(TerminalGraph, "Delete local variable");

		if (DeleteNodes == EResult::Yes)
		{
			for (UVoxelGraphNode_LocalVariable* Node : LocalVariableNodes)
			{
				Node->GetGraph()->RemoveNode(Node);
			}
		}
		else
		{
			// Reset GUIDs to avoid invalid references
			for (UVoxelGraphNode_LocalVariable* Node : LocalVariableNodes)
			{
				Node->Guid = {};
			}
		}

		TerminalGraph->RemoveLocalVariable(MemberGuid);
	}

	MarkAsDeleted();

	if (const TSharedPtr<SVoxelGraphMembers> Members = GetMembers())
	{
		Members->SelectClosestAction(GetSectionID(), MemberGuid);
	}
}

void FVoxelMembersSchemaAction_LocalVariable::OnDuplicate() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(MemberGuid);
	if (!ensure(LocalVariable))
	{
		return;
	}

	const FGuid NewGuid = FGuid::NewGuid();

	{
		const FVoxelTransaction Transaction(TerminalGraph, "Duplicate local variable");
		TerminalGraph->AddLocalVariable(NewGuid, MakeCopy(*LocalVariable));
	}

	Toolkit->GetSelection().SelectLocalVariable(*TerminalGraph, NewGuid);
	Toolkit->GetSelection().RequestRename();
}

bool FVoxelMembersSchemaAction_LocalVariable::OnCopy(FString& OutText) const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(MemberGuid);
	if (!ensure(LocalVariable))
	{
		return false;
	}

	const FVoxelGraphLocalVariable Defaults;

	FVoxelGraphLocalVariable::StaticStruct()->ExportText(
		OutText,
		LocalVariable,
		&Defaults,
		nullptr,
		0,
		nullptr);
	return true;
}

FString FVoxelMembersSchemaAction_LocalVariable::GetName() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return {};
	}

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(MemberGuid);
	if (!ensure(LocalVariable))
	{
		return {};
	}

	return LocalVariable->Name.ToString();
}

void FVoxelMembersSchemaAction_LocalVariable::SetName(const FString& Name) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Rename local variable");

	TerminalGraph->UpdateLocalVariable(MemberGuid, [&](FVoxelGraphLocalVariable& LocalVariable)
	{
		LocalVariable.Name = *Name;
	});
}

void FVoxelMembersSchemaAction_LocalVariable::SetCategory(const FString& NewCategory) const
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

	const FVoxelTransaction Transaction(TerminalGraph, "Set local variable category");

	TerminalGraph->UpdateLocalVariable(MemberGuid, [&](FVoxelGraphLocalVariable& LocalVariable)
	{
		LocalVariable.Category = *NewCategory;
	});
}

FString FVoxelMembersSchemaAction_LocalVariable::GetSearchString() const
{
	return MemberGuid.ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType FVoxelMembersSchemaAction_LocalVariable::GetPinType() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return {};
	}

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(MemberGuid);
	if (!ensure(LocalVariable))
	{
		return {};
	}

	return LocalVariable->Type;
}

void FVoxelMembersSchemaAction_LocalVariable::SetPinType(const FVoxelPinType& NewPinType) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Change local variable type");

	TerminalGraph->UpdateLocalVariable(MemberGuid, [&](FVoxelGraphLocalVariable& LocalVariable)
	{
		LocalVariable.Type = NewPinType;
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_LocalVariable::HoverTargetChanged()
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

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(LocalVariableGuid);
	if (!ensure(LocalVariable))
	{
		return;
	}

	if (const UEdGraphPin* Pin = GetHoveredPin())
	{
		if (!CheckPin(Pin, { EGPD_Input, EGPD_Output }))
		{
			return;
		}

		if (Pin->Direction == EGPD_Input)
		{
			if (!LocalVariable->Type.CanBeCastedTo_Schema(Pin->PinType))
			{
				SetFeedbackMessageError("The type of '" + LocalVariable->Name.ToString() + "' is not compatible with " + Pin->PinName.ToString());
				return;
			}

			SetFeedbackMessageOK("Make " + Pin->PinName.ToString() + " = " + LocalVariable->Name.ToString() + "");
			return;
		}
		else
		{
			if (!FVoxelPinType(Pin->PinType).CanBeCastedTo_Schema(LocalVariable->Type))
			{
				SetFeedbackMessageError("The type of '" + LocalVariable->Name.ToString() + "' is not compatible with " + Pin->PinName.ToString());
				return;
			}

			SetFeedbackMessageOK("Make " + LocalVariable->Name.ToString() + " = " + Pin->PinName.ToString() + "");
			return;
		}
	}

	if (Cast<UVoxelGraphNode_LocalVariable>(GetHoveredNode()))
	{
		SetFeedbackMessageOK("Change node local variable");
		return;
	}

	FVoxelMembersDragDropAction_Base::HoverTargetChanged();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FReply FVoxelMembersDragDropAction_LocalVariable::DroppedOnPin(const FVector2D ScreenPosition, const FVector2D GraphPosition)
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

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(LocalVariableGuid);
	if (!ensure(LocalVariable))
	{
		return FReply::Unhandled();
	}

	UEdGraphPin* Pin = GetHoveredPin();
	if (!ensure(Pin))
	{
		return FReply::Unhandled();
	}

	if (Pin->bOrphanedPin ||
		Pin->bNotConnectable)
	{
		return FReply::Unhandled();
	}

	if (Pin->Direction == EGPD_Input)
	{
		if (!LocalVariable->Type.CanBeCastedTo_Schema(Pin->PinType))
		{
			return FReply::Unhandled();
		}

		FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
		Action.Guid = LocalVariableGuid;
		Action.bIsDeclaration = false;
		Action.PerformAction(Pin->GetOwningNode()->GetGraph(), Pin, GraphPosition, true);

		return FReply::Handled();
	}
	else
	{
		if (!FVoxelPinType(Pin->PinType).CanBeCastedTo_Schema(LocalVariable->Type))
		{
			return FReply::Unhandled();
		}

		FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
		Action.Guid = LocalVariableGuid;
		Action.bIsDeclaration = true;
		Action.PerformAction(Pin->GetOwningNode()->GetGraph(), Pin, GraphPosition, true);

		return FReply::Handled();
	}
}

FReply FVoxelMembersDragDropAction_LocalVariable::DroppedOnNode(const FVector2D ScreenPosition, const FVector2D GraphPosition)
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

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(LocalVariableGuid);
	if (!ensure(LocalVariable))
	{
		return FReply::Unhandled();
	}

	UEdGraphNode* Node = GetHoveredNode();
	if (!ensure(Node))
	{
		return FReply::Unhandled();
	}

	UVoxelGraphNode_LocalVariable* LocalVariableNode = Cast<UVoxelGraphNode_LocalVariable>(Node);
	if (!LocalVariableNode)
	{
		return FReply::Unhandled();
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Replace local variable");
	LocalVariableNode->Guid = LocalVariableGuid;
	LocalVariableNode->CachedLocalVariable = *LocalVariable;
	LocalVariableNode->ReconstructNode();

	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_LocalVariable::DroppedOnPanel(
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

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(LocalVariableGuid);
	if (!ensure(LocalVariable))
	{
		return FReply::Unhandled();
	}

	const FModifierKeysState ModifierKeys = FSlateApplication::Get().GetModifierKeys();
	const bool bModifiedKeysActive = ModifierKeys.IsControlDown() || ModifierKeys.IsAltDown();
	const bool bAutoCreateGetter = bModifiedKeysActive ? ModifierKeys.IsControlDown() : bControlDrag;
	const bool bAutoCreateSetter = bModifiedKeysActive ? ModifierKeys.IsAltDown() : bAltDrag;

	if (bAutoCreateGetter)
	{
		FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
		Action.Guid = LocalVariableGuid;
		Action.bIsDeclaration = false;
		Action.PerformAction(&EdGraph, nullptr, GraphPosition, true);
		return FReply::Handled();
	}

	if (bAutoCreateSetter)
	{
		FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
		Action.Guid = LocalVariableGuid;
		Action.bIsDeclaration = true;
		Action.PerformAction(&EdGraph, nullptr, GraphPosition, true);
		return FReply::Handled();
	}

	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.BeginSection("VariableDroppedOn", FText::FromName(LocalVariable->Name));
	{
		MenuBuilder.AddMenuEntry(
			FText::FromString("Get " + LocalVariable->Name.ToString()),
			FText::FromString("Create Getter for local variable '" + LocalVariable->Name.ToString() + "'\n(Ctrl-drag to automatically create a getter)"),
			FSlateIcon(),
			FUIAction(MakeLambdaDelegate([Guid = LocalVariableGuid, Position = GraphPosition, WeakGraph = MakeWeakObjectPtr(&EdGraph)]
			{
				UEdGraph* PinnedEdGraph = WeakGraph.Get();
				if (!ensure(PinnedEdGraph))
				{
					return;
				}

				FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
				Action.Guid = Guid;
				Action.bIsDeclaration = false;
				Action.PerformAction(PinnedEdGraph, nullptr, Position, true);
			}))
		);

		MenuBuilder.AddMenuEntry(
			FText::FromString("Set " + LocalVariable->Name.ToString()),
			FText::FromString("Create Setter for local variable '" + LocalVariable->Name.ToString() + "'\n(Alt-drag to automatically create a setter)"),
			FSlateIcon(),
			FUIAction(MakeLambdaDelegate([Guid = LocalVariableGuid, Position = GraphPosition, WeakGraph = MakeWeakObjectPtr(&EdGraph)]
			{
				UEdGraph* PinnedEdGraph = WeakGraph.Get();
				if (!ensure(PinnedEdGraph))
				{
					return;
				}

				FVoxelGraphSchemaAction_NewLocalVariableUsage Action;
				Action.Guid = Guid;
				Action.bIsDeclaration = true;
				Action.PerformAction(PinnedEdGraph, nullptr, Position, true);
			}))
		);
	}
	MenuBuilder.EndSection();

	FSlateApplication::Get().PushMenu(
		Panel,
		FWidgetPath(),
		MenuBuilder.MakeWidget(),
		ScreenPosition,
		FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
	);

	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_LocalVariable::GetDefaultStatusSymbol(
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

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(LocalVariableGuid);
	if (!ensure(LocalVariable))
	{
		return;
	}

	PrimaryBrushOut = FVoxelGraphVisuals::GetPinIcon(LocalVariable->Type).GetIcon();
	IconColorOut = FVoxelGraphVisuals::GetPinColor(LocalVariable->Type);
}