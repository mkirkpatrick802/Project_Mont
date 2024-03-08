// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "GraphEditorDragDropAction.h"

class SVoxelMaterialDefinitionParameters;
struct FVoxelMaterialDefinitionParameterSchemaAction;

class FVoxelMaterialDefinitionParameterDragDropAction : public FGraphSchemaActionDragDropAction
{
public:
	DRAG_DROP_OPERATOR_TYPE(FVoxelMaterialDefinitionParameterDragDropAction, FGraphSchemaActionDragDropAction);

	static TSharedRef<FVoxelMaterialDefinitionParameterDragDropAction> New(
		const TSharedRef<SVoxelMaterialDefinitionParameters>& Parameters,
		const TSharedRef<FVoxelMaterialDefinitionParameterSchemaAction>& Action);

	//~ Begin FGraphSchemaActionDragDropAction Interface
	virtual void HoverTargetChanged() override;
	virtual FReply DroppedOnCategory(FText Category) override;
	virtual FReply DroppedOnAction(TSharedRef<FEdGraphSchemaAction> TargetAction) override;
	//~ End FGraphSchemaActionDragDropAction Interface

protected:
	TWeakPtr<SVoxelMaterialDefinitionParameters> WeakParameters;
	TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> DraggedAction;

	void SetFeedbackMessageError(const FString& Message);
	void SetFeedbackMessageOK(const FString& Message);

private:
	using FGraphSchemaActionDragDropAction::SourceAction;
	using FGraphSchemaActionDragDropAction::Construct;
};