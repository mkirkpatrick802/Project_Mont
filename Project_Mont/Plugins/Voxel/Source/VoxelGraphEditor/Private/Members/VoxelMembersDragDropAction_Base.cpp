// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Members/VoxelMembersDragDropAction_Base.h"
#include "Members/SVoxelGraphMembers.h"
#include "Members/VoxelMembersSchemaAction_Base.h"
#include "VoxelEdGraph.h"

TSharedRef<FVoxelMembersDragDropAction_Base> FVoxelMembersDragDropAction_Base::New(const TSharedRef<FVoxelMembersSchemaAction_Base>& Action)
{
	TSharedRef<FVoxelMembersDragDropAction_Base> Operation = MakeVoxelShared<FVoxelMembersDragDropAction_Base>();
	Operation->Construct(Action);
	return Operation;
}

void FVoxelMembersDragDropAction_Base::HoverTargetChanged()
{
	if (!DraggedAction)
	{
		FGraphSchemaActionDragDropAction::HoverTargetChanged();
		return;
	}

	const FString HoveredCategory = FVoxelUtilities::SanitizeCategory(HoveredCategoryName.ToString());
	ensure(FVoxelUtilities::ParseCategory(HoveredCategory).Num() == 1);

	if (!HoveredCategory.IsEmpty())
	{
		if (HoveredCategory == FVoxelUtilities::ParseCategory(DraggedAction->GetCategory()).Last())
		{
			SetFeedbackMessageError("'" + DraggedAction->GetMenuDescription().ToString() + "' is already in category '" + HoveredCategory + "'");
		}
		else
		{
			SetFeedbackMessageOK("Move '" + DraggedAction->GetMenuDescription().ToString() + "' to category '" + HoveredCategory + "'");
		}
		return;
	}

	const TSharedPtr<FEdGraphSchemaAction> TargetAction = HoveredAction.Pin();
	if (!TargetAction)
	{
		FGraphSchemaActionDragDropAction::HoverTargetChanged();
		return;
	}

	if (TargetAction->SectionID != DraggedAction->SectionID)
	{
		SetFeedbackMessageError("Cannot reorder '" + DraggedAction->GetMenuDescription().ToString() + "' into a different section.");
		return;
	}

	if (DraggedAction->GetPersistentItemDefiningObject() != TargetAction->GetPersistentItemDefiningObject())
	{
		SetFeedbackMessageError("Cannot reorder '" + DraggedAction->GetMenuDescription().ToString() + "' into a different scope.");
		return;
	}

	if (TargetAction == DraggedAction)
	{
		SetFeedbackMessageError("Cannot reorder '" + DraggedAction->GetMenuDescription().ToString() + "' before itself.");
		return;
	}

	SetFeedbackMessageOK("Reorder '" + DraggedAction->GetMenuDescription().ToString() + "' before '" + TargetAction->GetMenuDescription().ToString() + "'");
}

FReply FVoxelMembersDragDropAction_Base::DroppedOnCategory(const FText Category)
{
	if (!ensure(DraggedAction))
	{
		return FReply::Unhandled();
	}

	DraggedAction->MovePersistentItemToCategory(Category);
	return FReply::Handled();
}

FReply FVoxelMembersDragDropAction_Base::DroppedOnAction(const TSharedRef<FEdGraphSchemaAction> TargetAction)
{
	if (!ensure(DraggedAction.IsValid()) ||
		DraggedAction->GetTypeId() != TargetAction->GetTypeId() ||
		DraggedAction->GetPersistentItemDefiningObject() != TargetAction->GetPersistentItemDefiningObject() ||
		DraggedAction->SectionID != TargetAction->SectionID)
	{
		return FReply::Unhandled();
	}

	const FVoxelMembersSchemaAction_Base& TypedSourceAction = static_cast<const FVoxelMembersSchemaAction_Base&>(*DraggedAction);
	const FVoxelMembersSchemaAction_Base& TypedTargetAction = static_cast<const FVoxelMembersSchemaAction_Base&>(*TargetAction);

	const TSharedPtr<SVoxelGraphMembers> Members = TypedSourceAction.WeakMembers.Pin();
	if (!ensure(Members))
	{
		return FReply::Unhandled();
	}

	const FVoxelTransaction Transaction(TypedSourceAction.GetOuter(), "Reorder members");

	TArray<FGuid> Guids;
	for (const TSharedRef<FVoxelMembersSchemaAction_Base>& Action : Members->GetActions())
	{
		if (Action->SectionID != TypedSourceAction.SectionID)
		{
			continue;
		}

		ensure(!Guids.Contains(Action->MemberGuid));
		Guids.Add(Action->MemberGuid);
	}

	const int32 SourceIndex = Guids.Find(TypedSourceAction.MemberGuid);
	const int32 TargetIndex = Guids.Find(TypedTargetAction.MemberGuid);
	if (!ensure(SourceIndex != -1) ||
		!ensure(TargetIndex != -1))
	{
		return FReply::Unhandled();
	}

	TypedSourceAction.SetCategory(TypedTargetAction.GetCategory());

	Guids.RemoveAt(SourceIndex);

	int32 InsertIndex = TargetIndex;
	if (SourceIndex < TargetIndex)
	{
		InsertIndex--;
	}
	Guids.Insert(TypedSourceAction.MemberGuid, InsertIndex);

	TypedSourceAction.ApplyNewGuids(Guids);

	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Base::Construct(const TSharedRef<FVoxelMembersSchemaAction_Base>& NewDraggedAction)
{
	SourceAction = NewDraggedAction;
	DraggedAction = NewDraggedAction;
	FGraphEditorDragDropAction::Construct();
}

void FVoxelMembersDragDropAction_Base::SetFeedbackMessageError(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

void FVoxelMembersDragDropAction_Base::SetFeedbackMessageOK(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

bool FVoxelMembersDragDropAction_Base::CheckPin(
	const UEdGraphPin* Pin,
	const TArray<EEdGraphPinDirection>& ValidDirections)
{
	if (Pin->bOrphanedPin)
	{
		SetFeedbackMessageError("Cannot make connection to orphaned pin " + Pin->PinName.ToString() + "");
		return false;
	}
	if (Pin->bNotConnectable)
	{
		SetFeedbackMessageError("Cannot connect to pin");
		return false;
	}

	if (!ValidDirections.Contains(Pin->Direction))
	{
		if (Pin->Direction == EGPD_Input)
		{
			SetFeedbackMessageError("Cannot connect to input pin");
		}
		else
		{
			check(Pin->Direction == EGPD_Output);
			SetFeedbackMessageError("Cannot connect to output pin");
		}
		return false;
	}

	return true;
}