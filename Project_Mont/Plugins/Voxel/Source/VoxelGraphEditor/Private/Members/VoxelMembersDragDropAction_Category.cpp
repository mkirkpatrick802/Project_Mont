// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Members/VoxelMembersDragDropAction_Category.h"
#include "VoxelGraphToolkit.h"

void FVoxelMembersDragDropAction_Category::HoverTargetChanged()
{
	const TArray<FString> DraggedCategories = FVoxelUtilities::ParseCategory(DraggedCategory);
	if (HoveredCategoryName.IsEmpty())
	{
		if (HoveredAction.IsValid())
		{
			SetFeedbackMessageError("Can only insert before another category");
		}
		else
		{
			SetFeedbackMessageError("Moving category '" + DraggedCategories.Last() + "'");
		}
		return;
	}

	const FString HoveredCategory = FVoxelUtilities::SanitizeCategory(HoveredCategoryName.ToString());
	ensure(FVoxelUtilities::ParseCategory(HoveredCategory).Num() == 1);

	if (DraggedCategories.Last() == HoveredCategory)
	{
		SetFeedbackMessageError("Cannot insert category '" + DraggedCategories.Last() + "' before itself");
		return;
	}

	SetFeedbackMessageOK("Move category '" + DraggedCategories.Last() + "' before '" + HoveredCategory + "'");
}

FReply FVoxelMembersDragDropAction_Category::DroppedOnCategory(const FText DroppedOnCategory)
{
	const TSharedPtr<SVoxelGraphMembers> Members = WeakMembers.Pin();
	if (!ensure(Members))
	{
		return FReply::Handled();
	}

	const TSharedPtr<FVoxelGraphToolkit> Toolkit = Members->GetToolkit();
	if (!ensure(Toolkit))
	{
		return FReply::Handled();
	}

	if (DraggedCategory == DroppedOnCategory.ToString())
	{
		return FReply::Handled();
	}

	const FVoxelTransaction Transaction(Toolkit->Asset, "Reorder categories");

	TArray<FString> NewCategories;
	{
		const TArray<FString> DraggedCategories = FVoxelUtilities::ParseCategory(DraggedCategory);

		NewCategories = FVoxelUtilities::ParseCategory(DroppedOnCategory.ToString());
		NewCategories.Pop(false);
		NewCategories.Add(DraggedCategories.Last());
	}

	TArray<TSharedRef<FVoxelMembersSchemaAction_Base>> Actions;
	TArray<TSharedRef<FVoxelMembersSchemaAction_Base>> DraggedActions;
	for (const TSharedRef<FVoxelMembersSchemaAction_Base>& Action : Members->GetActions())
	{
		if (Action->SectionID != SectionId)
		{
			continue;
		}

		if (FVoxelUtilities::IsSubCategory(DraggedCategory, Action->GetCategory()))
		{
			DraggedActions.Add(Action);
		}
		else
		{
			Actions.Add(Action);
		}
	}

	if (!ensure(Actions.Num() > 0))
	{
		return FReply::Unhandled();
	}

	TArray<FGuid> NewGuids;
	bool bAreDraggedActionsAdded = false;
	for (const TSharedRef<FVoxelMembersSchemaAction_Base>& Action : Actions)
	{
		if (FVoxelUtilities::IsSubCategory(DroppedOnCategory.ToString(), Action->GetCategory()) &&
			!bAreDraggedActionsAdded)
		{
			bAreDraggedActionsAdded = true;

			for (const TSharedRef<FVoxelMembersSchemaAction_Base>& DraggedAction : DraggedActions)
			{
				NewGuids.Add(DraggedAction->MemberGuid);
			}
		}

		NewGuids.Add(Action->MemberGuid);
	}
	ensure(bAreDraggedActionsAdded);

	for (const TSharedRef<FVoxelMembersSchemaAction_Base>& DraggedAction : DraggedActions)
	{
		TArray<FString> Categories = FVoxelUtilities::ParseCategory(DraggedAction->GetCategory());
		TArray<FString> DraggedCategories = FVoxelUtilities::ParseCategory(DraggedCategory);

		for (const FString& Category : DraggedCategories)
		{
			if (ensure(Categories.Num() > 0 && Categories[0] == Category))
			{
				Categories.RemoveAt(0);
			}
		}

		TArray<FString> ActionNewCategories = NewCategories;
		ActionNewCategories.Append(Categories);
		DraggedAction->SetCategory(FVoxelUtilities::MakeCategory(ActionNewCategories));
	}

	Actions[0]->ApplyNewGuids(NewGuids);

	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Category::SetFeedbackMessageError(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

void FVoxelMembersDragDropAction_Category::SetFeedbackMessageOK(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}