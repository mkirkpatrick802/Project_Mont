// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"

struct FVoxelPinTypeSelectorRow;
using SVoxelPinTypeTreeView = STreeView<TSharedPtr<FVoxelPinTypeSelectorRow>>;

struct FVoxelPinTypeSelectorRow
{
public:
	FString Name;
	FVoxelPinType Type;
	TArray<TSharedPtr<FVoxelPinTypeSelectorRow>> Children;

	FVoxelPinTypeSelectorRow() = default;

	explicit FVoxelPinTypeSelectorRow(const FVoxelPinType& PinType)
		: Name(PinType.GetInnerType().ToString())
		, Type(PinType)
	{
	}

	explicit FVoxelPinTypeSelectorRow(const FString& Name)
		: Name(Name)
	{
	}

	FVoxelPinTypeSelectorRow(
		const FString& Name,
		const TArray<TSharedPtr<FVoxelPinTypeSelectorRow>>& Children)
		: Name(Name)
		, Children(Children)
	{
	}
};

class VOXELGRAPHEDITOR_API SVoxelPinTypeSelector : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnTypeChanged, FVoxelPinType)

public:
	VOXEL_SLATE_ARGS()
	{
		FArguments()
			: _AllowWildcard(false)
		{}
		SLATE_ATTRIBUTE(FVoxelPinTypeSet, AllowedTypes)
		SLATE_ARGUMENT(bool, AllowWildcard)
		SLATE_EVENT(FOnTypeChanged, OnTypeChanged)
		SLATE_EVENT(FSimpleDelegate, OnCloseMenu)
	};

	void Construct(const FArguments& InArgs);

public:
	void ClearSelection() const;
	TSharedPtr<SWidget> GetWidgetToFocus() const;

private:
	TSharedRef<ITableRow> GenerateTypeTreeRow(TSharedPtr<FVoxelPinTypeSelectorRow> RowItem, const TSharedRef<STableViewBase>& OwnerTable) const;
	void OnTypeSelectionChanged(TSharedPtr<FVoxelPinTypeSelectorRow> Selection, ESelectInfo::Type SelectInfo) const;
	void GetTypeChildren(TSharedPtr<FVoxelPinTypeSelectorRow> PinTypeRow, TArray<TSharedPtr<FVoxelPinTypeSelectorRow>>& PinTypeRows) const;

	void OnFilterTextChanged(const FText& NewText);
	void OnFilterTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo) const;

private:
	void GetChildrenMatchingSearch(const FText& InSearchText);
	void FillTypesList();
	const FSlateBrush* GetIcon(const FVoxelPinType& PinType) const;
	FLinearColor GetColor(const FVoxelPinType& PinType) const;

private:
	TSharedPtr<SVoxelPinTypeTreeView> TypeTreeView;
	TSharedPtr<SSearchBox> FilterTextBox;

private:
	TAttribute<FVoxelPinTypeSet> AllowedTypes;
	bool bAllowWildcard = false;
	FOnTypeChanged OnTypeChanged;
	FSimpleDelegate OnCloseMenu;

private:
	FText SearchText;

	TArray<TSharedPtr<FVoxelPinTypeSelectorRow>> TypesList;
	TArray<TSharedPtr<FVoxelPinTypeSelectorRow>> FilteredTypesList;

	int32 FilteredTypesCount = 0;
	int32 TotalTypesCount = 0;
};