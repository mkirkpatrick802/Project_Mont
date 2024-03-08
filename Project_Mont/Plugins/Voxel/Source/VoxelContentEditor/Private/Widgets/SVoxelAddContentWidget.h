// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Misc/TextFilter.h"
#include "VoxelExampleContentManager.h"

class SVoxelAddContentWidget : public SCompoundWidget
{
public:
	typedef TTextFilter<TSharedPtr<FVoxelExampleContent>> FContentSourceTextFilter;

public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs);

private:
	FReply AddButtonClicked();
	FReply CancelButtonClicked();

	void SearchTextChanged(const FText& Text);

	TSharedRef<SWidget> CreateTagsFilterBox();
	TSharedRef<SWidget> CreateContentTilesView();
	TSharedRef<SWidget> CreateContentDetails();

	TSharedRef<ITableRow> CreateContentSourceIconTile(TSharedPtr<FVoxelExampleContent> Content, const TSharedRef<STableViewBase>& OwnerTable) const;

private:
	TSharedPtr<SBox> CategoryTabsBox;
	TSharedPtr<SWrapBox> SelectedContentTagsBox;
	TSharedPtr<SSearchBox> SearchBox;
	TMap<FName, TSharedPtr<STileView<TSharedPtr<FVoxelExampleContent>>>> CategoryToTilesView;

	TArray<TSharedPtr<FVoxelExampleContent>> ContentsSourceList;
	TMap<FName, TArray<TSharedPtr<FVoxelExampleContent>>> CategoryToFilteredList;

	TSet<FName> Categories;
	TSet<FName> Tags;
	TSet<FName> VisibleTags;

	TSharedPtr<FVoxelExampleContent> SelectedContent;

	TSharedPtr<FContentSourceTextFilter> Filter;

	void GatherContent();
	void UpdateFilteredItems();
	void UpdateContentTags(const TSharedPtr<FVoxelExampleContent>& OldContent) const;
	void FinalizeDownload(const TArray<uint8>& InData) const;
};