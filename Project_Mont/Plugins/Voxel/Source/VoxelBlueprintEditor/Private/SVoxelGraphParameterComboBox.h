// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelGraphParameterSelector.h"

class SVoxelGraphParameterSelector;

class SVoxelGraphParameterComboBox : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(TArray<FVoxelSelectorItem>, Items)
		SLATE_ATTRIBUTE(FVoxelSelectorItem, CurrentItem)
		SLATE_ATTRIBUTE(bool, IsValidItem)
		SLATE_EVENT(TDelegate<void(const FVoxelSelectorItem&)>, OnItemChanged)
	};

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> GetMenuContent();

private:
	TSharedPtr<SComboButton> TypeComboButton;

	TSharedPtr<SMenuOwner> MenuContent;
	TSharedPtr<SVoxelGraphParameterSelector> ParameterSelector;

private:
	TDelegate<void(const FVoxelSelectorItem&)> OnItemChanged;
	TAttribute<TArray<FVoxelSelectorItem>> Items;
	TAttribute<FVoxelSelectorItem> CurrentItem;
	TAttribute<bool> bIsValidItem;
};