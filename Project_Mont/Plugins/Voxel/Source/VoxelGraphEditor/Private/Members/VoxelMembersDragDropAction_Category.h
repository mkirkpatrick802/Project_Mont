// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Members/SVoxelGraphMembers.h"
#include "GraphEditorDragDropAction.h"

class FVoxelMembersDragDropAction_Category : public FGraphEditorDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FVoxelMembersCategoryDragDropAction, FGraphEditorDragDropAction);

	static TSharedRef<FVoxelMembersDragDropAction_Category> New(
		const FString& InCategory,
		const TSharedPtr<SVoxelGraphMembers>& Members,
		const int32 SectionId)
	{
		const TSharedRef<FVoxelMembersDragDropAction_Category> Operation = MakeVoxelShared<FVoxelMembersDragDropAction_Category>();
		Operation->DraggedCategory = InCategory;
		Operation->SectionId = SectionId;
		Operation->WeakMembers = Members;
		Operation->Construct();
		return Operation;
	}

	//~ Begin FGraphSchemaActionDragDropAction Interface
	virtual void HoverTargetChanged() override;
	virtual FReply DroppedOnCategory(FText DroppedOnCategory) override;
	//~ End FGraphSchemaActionDragDropAction Interface

private:
	int32 SectionId = 0;
	FString DraggedCategory;
	TWeakPtr<SVoxelGraphMembers> WeakMembers;

	void SetFeedbackMessageError(const FString& Message);
	void SetFeedbackMessageOK(const FString& Message);
};