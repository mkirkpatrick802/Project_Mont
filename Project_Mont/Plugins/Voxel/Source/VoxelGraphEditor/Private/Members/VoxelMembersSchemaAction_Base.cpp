// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Members/VoxelMembersSchemaAction_Base.h"

FName FVoxelMembersSchemaAction_Base::GetTypeId() const
{
	return StaticGetTypeId();
}

FName FVoxelMembersSchemaAction_Base::StaticGetTypeId()
{
	return STATIC_FNAME("FVoxelMembersSchemaAction_Base");
}

FString FVoxelMembersSchemaAction_Base::GetCategory() const
{
	return FVoxelUtilities::SanitizeCategory(FEdGraphSchemaAction::GetCategory().ToString());
}

void FVoxelMembersSchemaAction_Base::MarkAsDeleted() const
{
	ensure(!bIsDeleted);
	bIsDeleted = true;
}

FVoxelMembersSchemaAction_Base::EResult FVoxelMembersSchemaAction_Base::CreateDeletePopups(const FString& Title, const FString& ObjectName)
{
	if (EAppReturnType::No == FMessageDialog::Open(
		EAppMsgType::YesNo,
		EAppReturnType::No,
		FText::FromString(ObjectName + " is in use, are you sure you want to delete it?"),
		FText::FromString(Title)))
	{
		return EResult::Cancel;
	}

	const EAppReturnType::Type DialogResult = FMessageDialog::Open(
		EAppMsgType::YesNoCancel,
		EAppReturnType::Cancel,
		FText::FromString("Do you want to delete its references from the graph?"),
		FText::FromString("Delete " + ObjectName + " usages"));

	if (DialogResult == EAppReturnType::Cancel)
	{
		return EResult::Cancel;
	}

	if (DialogResult == EAppReturnType::Yes)
	{
		return EResult::Yes;
	}

	ensure(DialogResult == EAppReturnType::No);
	return EResult::No;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelMembersPaletteItem_Base::CreateTextSlotWidget(const FCreateWidgetForActionData& CreateData)
{
	return CreateTextSlotWidget(ConstCast(&CreateData), MakeAttributeLambda([WeakAction = MakeWeakPtr(CreateData.Action)]
	{
		const TSharedPtr<FEdGraphSchemaAction> Action = WeakAction.Pin();
		if (!ensure(Action))
		{
			return false;
		}

		return !Action->CanBeRenamed();
	}));
}

void SVoxelMembersPaletteItem_Base::OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetAction();
	if (!ensure(Action))
	{
		return;
	}

	Action->SetName(NewText.ToString());
}

FText SVoxelMembersPaletteItem_Base::GetDisplayText() const
{
	const TSharedPtr<FVoxelMembersSchemaAction_Base> Action = GetAction();
	if (!ensure(Action) ||
		Action->IsDeleted())
	{
		return {};
	}

	return FText::FromString(Action->GetName());
}

TSharedPtr<FVoxelMembersSchemaAction_Base> SVoxelMembersPaletteItem_Base::GetAction() const
{
	const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
	if (!Action)
	{
		return nullptr;
	}

	return StaticCastSharedPtr<FVoxelMembersSchemaAction_Base>(Action);
}