// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphToolkit.h"

class SSearchBox;
class FVoxelGraphSearchItem;
struct FVoxelGraphToolkit;

enum class EVoxelGraphSearchResultTag
{
	Special,
	Graph,
	TerminalGraph,
	Node,
	NodeTitle,
	Pin,
	Parameter,
	Input,
	Output,
	LocalVariable,
	Type,
	Description,
	DefaultValue,
};

struct FVoxelGraphSearchSettings
{
	bool bSearchNodes = true;
	bool bSearchPins = true;
	bool bSearchParameters = true;
	bool bSearchTypes = true;
	bool bSearchDescriptions = true;
	bool bSearchDefaultValues = true;

	bool bRemoveSpaces = false;

	void LoadFromIni();
	bool ShowTag(EVoxelGraphSearchResultTag Tag) const;
};

class SVoxelGraphSearch : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		FArguments()
			: _IsGlobalSearch(false)
		{
		}

		SLATE_ARGUMENT(TSharedPtr<FVoxelGraphToolkit>, Toolkit);
		SLATE_ARGUMENT(bool, IsGlobalSearch);
	};

	void Construct(const FArguments& InArgs);
	void FocusForUse(const FString& NewSearchTerms);

private:
	TSharedRef<SWidget> CreateSearchSettingsWidget();
	TSharedRef<ITableRow> OnGenerateRow(TSharedPtr<FVoxelGraphSearchItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const;
	void MakeSearchQuery(const FString& SearchQuery);

private:
	using STreeViewType = STreeView<TSharedPtr<FVoxelGraphSearchItem>>;

	TWeakPtr<FVoxelGraphToolkit> WeakToolkit;
	bool bIsGlobalSearch = false;

	TSharedPtr<SSearchBox> SearchTextField;
	TSharedPtr<STreeViewType> TreeView;

	FString	SearchValue;
	FText HighlightText;

	TArray<TSharedPtr<FVoxelGraphSearchItem>> RootItems;

	FVoxelGraphSearchSettings SearchSettings;
};