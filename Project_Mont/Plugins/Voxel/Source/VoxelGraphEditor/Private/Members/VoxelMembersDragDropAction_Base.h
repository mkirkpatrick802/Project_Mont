// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "GraphEditorDragDropAction.h"

struct FVoxelMembersSchemaAction_Base;

class FVoxelMembersDragDropAction_Base : public FGraphSchemaActionDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FVoxelMembersDragDropAction_Base, FGraphSchemaActionDragDropAction);

	static TSharedRef<FVoxelMembersDragDropAction_Base> New(const TSharedRef<FVoxelMembersSchemaAction_Base>& Action);

	//~ Begin FGraphSchemaActionDragDropAction Interface
	virtual void HoverTargetChanged() override;
	virtual FReply DroppedOnCategory(FText Category) override;
	virtual FReply DroppedOnAction(TSharedRef<FEdGraphSchemaAction> TargetAction) override;
	//~ End FGraphSchemaActionDragDropAction Interface

protected:
	TSharedPtr<FVoxelMembersSchemaAction_Base> DraggedAction;

	void Construct(const TSharedRef<FVoxelMembersSchemaAction_Base>& NewDraggedAction);

	void SetFeedbackMessageError(const FString& Message);
	void SetFeedbackMessageOK(const FString& Message);

	bool CheckPin(
		const UEdGraphPin* Pin,
		const TArray<EEdGraphPinDirection>& ValidDirections);

private:
	using FGraphSchemaActionDragDropAction::SourceAction;
	using FGraphSchemaActionDragDropAction::Construct;
};