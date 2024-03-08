// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"

struct FSelectorItemRow;
typedef STreeView<TSharedPtr<FSelectorItemRow>> SItemsTreeView;

struct FVoxelSelectorItem
{
	FGuid Guid;
	FName Name;
	FVoxelPinType Type;
	FString Category;

	FVoxelSelectorItem() = default;
	FVoxelSelectorItem(const FGuid Guid, const FName Name, const FVoxelPinType& Type, const FString& Category)
		: Guid(Guid)
		, Name(Name)
		, Type(Type)
		, Category(Category)
	{}
	FVoxelSelectorItem(const FGuid Guid, const FName Name, const FVoxelPinType& Type)
		: Guid(Guid)
		, Name(Name)
		, Type(Type)
	{}
};

class SVoxelGraphParameterSelector : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(TArray<FVoxelSelectorItem>, Items)
		SLATE_EVENT(TDelegate<void(const FVoxelSelectorItem&)>, OnItemChanged)
		SLATE_EVENT(FSimpleDelegate, OnCloseMenu)
	};

	void Construct(const FArguments& InArgs);

public:
	void ClearSelection() const;
	TSharedPtr<SWidget> GetWidgetToFocus() const;
	void Refresh();

private:
	TSharedRef<ITableRow> GenerateItemTreeRow(TSharedPtr<FSelectorItemRow> RowItem, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnItemSelectionChanged(TSharedPtr<FSelectorItemRow> Selection, ESelectInfo::Type SelectInfo) const;
	void GetItemChildren(TSharedPtr<FSelectorItemRow> ItemRow, TArray<TSharedPtr<FSelectorItemRow>>& ChildrenRows) const;

	void OnFilterTextChanged(const FText& NewText);
	void OnFilterTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo) const;

private:
	void GetChildrenMatchingSearch(const FText& InSearchText);
	void FillItemsList();
	TArray<FString> GetCategoryChain(const FString& Category) const;

private:
	TSharedPtr<SItemsTreeView> ItemsTreeView;
	TSharedPtr<SSearchBox> FilterTextBox;

private:
	TDelegate<void(const FVoxelSelectorItem&)> OnItemChanged;
	FSimpleDelegate OnCloseMenu;
	TAttribute<TArray<FVoxelSelectorItem>> Items;

private:
	FText SearchText;

	TArray<TSharedPtr<FSelectorItemRow>> ItemsList;
	TArray<TSharedPtr<FSelectorItemRow>> FilteredItemsList;
	TWeakPtr<FSelectorItemRow> AutoExpandedRow;

	int32 FilteredItemsCount = 0;
	int32 TotalItemsCount = 0;
};