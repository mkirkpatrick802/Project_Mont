// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelNewAssetInstancePickerList.h"

class SVoxelNewGraphDialog : public SWindow
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_EVENT(SVoxelNewAssetInstancePickerList::FGetAssetText, OnGetAssetCategory)
		SLATE_EVENT(SVoxelNewAssetInstancePickerList::FGetAssetText, OnGetAssetDescription)
		SLATE_EVENT(SVoxelNewAssetInstancePickerList::FShowAsset, ShowAsset)
	};

	void Construct(const FArguments& InArgs, UClass* TargetClass);

public:
	bool GetUserConfirmedSelection() const
	{
		return bUserConfirmedSelection;
	}

	TOptional<FAssetData> GetSelectedAsset();

private:
	void ConfirmSelection(const FAssetData& AssetData);

	FReply OnConfirmSelection();
	bool IsCreateButtonEnabled() const;
	FReply OnCancelButtonClicked();
	FReply OnOpenContentExamplesClicked();

private:
	TArray<FAssetData> SelectedAssets;
	bool bUserConfirmedSelection = false;
	TSharedPtr<SVoxelNewAssetInstancePickerList> NewAssetPicker;
};