// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionParameterDragDropAction.h"
#include "Material/VoxelMaterialDefinition.h"
#include "Material/VoxelMaterialDefinitionToolkit.h"
#include "Material/SVoxelMaterialDefinitionParameters.h"
#include "Material/VoxelMaterialDefinitionParameterSchemaAction.h"

TSharedRef<FVoxelMaterialDefinitionParameterDragDropAction> FVoxelMaterialDefinitionParameterDragDropAction::New(
	const TSharedRef<SVoxelMaterialDefinitionParameters>& Parameters,
	const TSharedRef<FVoxelMaterialDefinitionParameterSchemaAction>& Action)
{
	TSharedRef<FVoxelMaterialDefinitionParameterDragDropAction> Operation = MakeVoxelShared<FVoxelMaterialDefinitionParameterDragDropAction>();
	Operation->WeakParameters = Parameters;
	Operation->SourceAction = Action;
	Operation->DraggedAction = Action;
	Operation->Construct();
	return Operation;
}

void FVoxelMaterialDefinitionParameterDragDropAction::HoverTargetChanged()
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
		if (HoveredCategory == FVoxelUtilities::ParseCategory(DraggedAction->GetCategory().ToString()).Last())
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

FReply FVoxelMaterialDefinitionParameterDragDropAction::DroppedOnCategory(const FText Category)
{
	if (!ensure(DraggedAction))
	{
		return FReply::Unhandled();
	}

	DraggedAction->MovePersistentItemToCategory(Category);
	return FReply::Handled();
}

FReply FVoxelMaterialDefinitionParameterDragDropAction::DroppedOnAction(const TSharedRef<FEdGraphSchemaAction> TargetAction)
{
	if (!ensure(DraggedAction.IsValid()) ||
		DraggedAction->GetTypeId() != TargetAction->GetTypeId() ||
		DraggedAction->GetPersistentItemDefiningObject() != TargetAction->GetPersistentItemDefiningObject() ||
		DraggedAction->SectionID != TargetAction->SectionID)
	{
		return FReply::Unhandled();
	}

	const FVoxelMaterialDefinitionParameterSchemaAction& TypedSourceAction = static_cast<const FVoxelMaterialDefinitionParameterSchemaAction&>(*DraggedAction);
	const FVoxelMaterialDefinitionParameterSchemaAction& TypedTargetAction = static_cast<const FVoxelMaterialDefinitionParameterSchemaAction&>(*TargetAction);

	const TSharedPtr<SVoxelMaterialDefinitionParameters> Parameters = WeakParameters.Pin();
	if (!ensure(Parameters))
	{
		return FReply::Unhandled();
	}

	const TSharedPtr<FVoxelMaterialDefinitionToolkit> Toolkit = Parameters->GetToolkit();
	if (!ensure(Toolkit))
	{
		return FReply::Unhandled();
	}
	UVoxelMaterialDefinition* MaterialDefinition = Toolkit->Asset;

	const FVoxelTransaction Transaction(MaterialDefinition, "Reorder members");

	TArray<FGuid> Guids;
	for (const auto& It : MaterialDefinition->GuidToMaterialParameter)
	{
		ensure(!Guids.Contains(It.Key));
		Guids.Add(It.Key);
	}

	const int32 SourceIndex = Guids.Find(TypedSourceAction.Guid);
	const int32 TargetIndex = Guids.Find(TypedTargetAction.Guid);
	if (!ensure(SourceIndex != -1) ||
		!ensure(TargetIndex != -1))
	{
		return FReply::Unhandled();
	}

	FVoxelMaterialDefinitionParameter* SourceParameter = MaterialDefinition->GuidToMaterialParameter.Find(TypedSourceAction.Guid);
	if (!ensure(SourceParameter))
	{
		return FReply::Unhandled();
	}
	SourceParameter->Category = TypedTargetAction.GetCategory().ToString();

	Guids.RemoveAt(SourceIndex);

	int32 InsertIndex = TargetIndex;
	if (SourceIndex < TargetIndex)
	{
		InsertIndex--;
	}
	Guids.Insert(TypedSourceAction.Guid, InsertIndex);

	FVoxelUtilities::ReorderMapKeys(MaterialDefinition->GuidToMaterialParameter, Guids);

	Parameters->QueueRefresh();
	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionParameterDragDropAction::SetFeedbackMessageError(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}

void FVoxelMaterialDefinitionParameterDragDropAction::SetFeedbackMessageOK(const FString& Message)
{
	const FSlateBrush* StatusSymbol = FAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, FText::FromString(Message));
}