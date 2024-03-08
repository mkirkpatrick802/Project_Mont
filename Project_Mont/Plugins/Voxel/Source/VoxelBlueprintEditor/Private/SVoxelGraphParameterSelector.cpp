// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphParameterSelector.h"
#include "VoxelGraphVisuals.h"
#include "SListViewSelectorDropdownMenu.h"

struct FSelectorItemRow
{
public:
	FString Name;
	FVoxelSelectorItem Item;
	TArray<TSharedPtr<FSelectorItemRow>> Items;
	TMap<FName, TSharedPtr<FSelectorItemRow>> ChildCategories;

	FSelectorItemRow() = default;

	explicit FSelectorItemRow(
		const FVoxelSelectorItem& Item)
		: Name(Item.Name.ToString())
		, Item(Item)
	{
	}

	explicit FSelectorItemRow(const FString& Name)
		: Name(Name)
	{
	}

	FSelectorItemRow(const FString& Name, const TArray<TSharedPtr<FSelectorItemRow>>& Types)
		: Name(Name)
		, Items(Types)
	{
	}
};

void SVoxelGraphParameterSelector::Construct(const FArguments& InArgs)
{
	OnItemChanged = InArgs._OnItemChanged;
	ensure(OnItemChanged.IsBound());
	OnCloseMenu = InArgs._OnCloseMenu;
	Items = InArgs._Items;

	FillItemsList();

	SAssignNew(ItemsTreeView, SItemsTreeView)
	.TreeItemsSource(&FilteredItemsList)
	.SelectionMode(ESelectionMode::Single)
	.OnGenerateRow(this, &SVoxelGraphParameterSelector::GenerateItemTreeRow)
	.OnSelectionChanged(this, &SVoxelGraphParameterSelector::OnItemSelectionChanged)
	.OnGetChildren(this, &SVoxelGraphParameterSelector::GetItemChildren);

	ItemsTreeView->SetSingleExpandedItem(AutoExpandedRow.Pin());

	SAssignNew(FilterTextBox, SSearchBox)
	.OnTextChanged(this, &SVoxelGraphParameterSelector::OnFilterTextChanged)
	.OnTextCommitted(this, &SVoxelGraphParameterSelector::OnFilterTextCommitted);

	ChildSlot
	[
		SNew(SListViewSelectorDropdownMenu<TSharedPtr<FSelectorItemRow>>, FilterTextBox, ItemsTreeView)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				FilterTextBox.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(SBox)
				.HeightOverride(400.f)
				.WidthOverride(300.f)
				[
					ItemsTreeView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f, 0.f, 8.f, 4.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]
				{
					const FString ItemText = FilteredItemsCount == 1 ? " parameter" : " parameters";
					return FText::FromString(FText::AsNumber(FilteredItemsCount).ToString() + ItemText);
				})
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphParameterSelector::ClearSelection() const
{
	ItemsTreeView->SetSelection(nullptr, ESelectInfo::OnNavigation);
	ItemsTreeView->SetSingleExpandedItem(AutoExpandedRow.Pin());
}

TSharedPtr<SWidget> SVoxelGraphParameterSelector::GetWidgetToFocus() const
{
	return FilterTextBox;
}

void SVoxelGraphParameterSelector::Refresh()
{
	FillItemsList();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<ITableRow> SVoxelGraphParameterSelector::GenerateItemTreeRow(TSharedPtr<FSelectorItemRow> RowItem, const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FLinearColor Color = RowItem->Items.Num() > 0 ? FLinearColor::White : FVoxelGraphVisuals::GetPinColor(RowItem->Item.Type);
	const FSlateBrush* Image = RowItem->Items.Num() > 0 ? FAppStyle::GetBrush("NoBrush") : FVoxelGraphVisuals::GetPinIcon(RowItem->Item.Type).GetIcon();

	const TSharedRef<SHorizontalBox> Row =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		  .AutoWidth()
		  .Padding(1.f)
		[
			SNew(SImage)
			.Image(Image)
			.ColorAndOpacity(Color)
		]
		+ SHorizontalBox::Slot()
		  .AutoWidth()
		  .Padding(1.f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(RowItem->Name))
			.HighlightText_Lambda([this]
			{
				return SearchText;
			})
			.Font(RowItem->Items.Num() > 0 ? FAppStyle::GetFontStyle(TEXT("Kismet.TypePicker.CategoryFont")) : FAppStyle::GetFontStyle(TEXT("Kismet.TypePicker.NormalFont")))
		];

	if (RowItem->Items.Num() == 0)
	{
		Row->AddSlot()
			.AutoWidth()
			.Padding(5.f, 1.f, 1.f, 1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString("(" + RowItem->Item.Type.ToString() + ")"))
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
	}

	return
		SNew(STableRow<TSharedPtr<FSelectorItemRow>>, OwnerTable)
		[
			Row
		];
}

void SVoxelGraphParameterSelector::OnItemSelectionChanged(TSharedPtr<FSelectorItemRow> Selection, const ESelectInfo::Type SelectInfo) const
{
	if (SelectInfo == ESelectInfo::OnNavigation)
	{
		if (ItemsTreeView->WidgetFromItem(Selection).IsValid())
		{
			OnCloseMenu.ExecuteIfBound();
		}

		return;
	}

	if (!Selection)
	{
		return;
	}

	if (Selection->Items.Num() == 0)
	{
		OnCloseMenu.ExecuteIfBound();
		OnItemChanged.ExecuteIfBound(Selection->Item);
		return;
	}

	ItemsTreeView->SetItemExpansion(Selection, !ItemsTreeView->IsItemExpanded(Selection));

	if (SelectInfo == ESelectInfo::OnMouseClick)
	{
		ItemsTreeView->ClearSelection();
	}
}

void SVoxelGraphParameterSelector::GetItemChildren(TSharedPtr<FSelectorItemRow> ItemRow, TArray<TSharedPtr<FSelectorItemRow>>& ChildrenRows) const
{
	ChildrenRows = ItemRow->Items;
}

void SVoxelGraphParameterSelector::OnFilterTextChanged(const FText& NewText)
{
	SearchText = NewText;
	FilteredItemsList = {};

	GetChildrenMatchingSearch(NewText);
	ItemsTreeView->RequestTreeRefresh();
}

void SVoxelGraphParameterSelector::OnFilterTextCommitted(const FText& NewText, const ETextCommit::Type CommitInfo) const
{
	if (CommitInfo != ETextCommit::OnEnter)
	{
		return;
	}

	TArray<TSharedPtr<FSelectorItemRow>> SelectedItems = ItemsTreeView->GetSelectedItems();
	if (SelectedItems.Num() > 0)
	{
		ItemsTreeView->SetSelection(SelectedItems[0]);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphParameterSelector::GetChildrenMatchingSearch(const FText& InSearchText)
{
	FilteredItemsCount = 0;

	TArray<FString> FilterTerms;
	TArray<FString> SanitizedFilterTerms;

	if (InSearchText.IsEmpty())
	{
		FilteredItemsList = ItemsList;
		FilteredItemsCount = TotalItemsCount;
		return;
	}

	FText::TrimPrecedingAndTrailing(InSearchText).ToString().ParseIntoArray(FilterTerms, TEXT(" "), true);

	for (int32 Index = 0; Index < FilterTerms.Num(); Index++)
	{
		FString EachString = FName::NameToDisplayString(FilterTerms[Index], false);
		EachString = EachString.Replace(TEXT(" "), TEXT(""));
		SanitizedFilterTerms.Add(EachString);
	}

	ensure(SanitizedFilterTerms.Num() == FilterTerms.Num());

	const auto SearchMatches = [&FilterTerms, &SanitizedFilterTerms](const TSharedPtr<FSelectorItemRow>& TypeRow) -> bool
	{
		FString ItemName = TypeRow->Name;
		ItemName = ItemName.Replace(TEXT(" "), TEXT(""));

		for (int32 Index = 0; Index < FilterTerms.Num(); ++Index)
		{
			if (ItemName.Contains(FilterTerms[Index]) ||
				ItemName.Contains(SanitizedFilterTerms[Index]))
			{
				return true;
			}
		}

		return false;
	};

	const auto LookThroughList = [&](const TArray<TSharedPtr<FSelectorItemRow>>& UnfilteredList, TArray<TSharedPtr<FSelectorItemRow>>& OutFilteredList, auto& Lambda) -> bool
	{
		bool bReturnVal = false;
		for (const TSharedPtr<FSelectorItemRow>& TypeRow : UnfilteredList)
		{
			const bool bMatchesItem = SearchMatches(TypeRow);
			if (TypeRow->Items.Num() == 0 ||
				bMatchesItem)
			{
				if (bMatchesItem)
				{
					OutFilteredList.Add(TypeRow);

					if (TypeRow->Items.Num() > 0 &&
						ItemsTreeView.IsValid())
					{
						ItemsTreeView->SetItemExpansion(TypeRow, true);
					}

					FilteredItemsCount += FMath::Max(1, TypeRow->Items.Num());

					bReturnVal = true;
				}
				continue;
			}

			TArray<TSharedPtr<FSelectorItemRow>> ValidChildren;
			if (Lambda(TypeRow->Items, ValidChildren, Lambda))
			{
				TSharedRef<FSelectorItemRow> NewCategory = MakeVoxelShared<FSelectorItemRow>(TypeRow->Name, ValidChildren);
				OutFilteredList.Add(NewCategory);

				if (ItemsTreeView.IsValid())
				{
					ItemsTreeView->SetItemExpansion(NewCategory, true);
				}

				bReturnVal = true;
			}
		}

		return bReturnVal;
	};

	LookThroughList(ItemsList, FilteredItemsList, LookThroughList);
}

void SVoxelGraphParameterSelector::FillItemsList()
{
	ItemsList = {};

	const TSharedPtr<FSelectorItemRow> RootRow = MakeVoxelShared<FSelectorItemRow>();

	{
		const TSharedPtr<FSelectorItemRow> DefaultCategory = MakeVoxelShared<FSelectorItemRow>("Default");
		RootRow->ChildCategories.Add(STATIC_FNAME("Default"), DefaultCategory);
		RootRow->Items.Add(DefaultCategory);
	}

	const auto GetCategoryRow = [&](const FString& Category) -> TSharedPtr<FSelectorItemRow>
	{
		TArray<FString> CategoryChain = GetCategoryChain(Category);
		if (CategoryChain.Num() == 0)
		{
			return RootRow->ChildCategories[STATIC_FNAME("Default")];
		}

		TSharedPtr<FSelectorItemRow> TargetRow = RootRow;
		for (int32 Index = 0; Index < CategoryChain.Num(); Index++)
		{
			if (const TSharedPtr<FSelectorItemRow> CategoryRow = TargetRow->ChildCategories.FindRef(*CategoryChain[Index]))
			{
				TargetRow = CategoryRow;
				continue;
			}

			TSharedPtr<FSelectorItemRow> CategoryRow = MakeVoxelShared<FSelectorItemRow>(CategoryChain[Index]);
			TargetRow->Items.Add(CategoryRow);
			TargetRow->ChildCategories.Add(*CategoryChain[Index], CategoryRow);

			TargetRow = CategoryRow;
		}

		return TargetRow;
	};

	int32 NumItems = 0;
	for (const FVoxelSelectorItem& Item : Items.Get())
	{
		if (const TSharedPtr<FSelectorItemRow> CategoryRow = GetCategoryRow(Item.Category))
		{
			CategoryRow->Items.Add(MakeVoxelShared<FSelectorItemRow>(Item));
		}
		else
		{
			ItemsList.Add(MakeVoxelShared<FSelectorItemRow>(Item));
		}

		NumItems++;
	}

	AutoExpandedRow = nullptr;
	for (const TSharedPtr<FSelectorItemRow>& CategoryRow : RootRow->Items)
	{
		if (CategoryRow->Items.Num() == 0)
		{
			continue;
		}

		if (!AutoExpandedRow.IsValid())
		{
			AutoExpandedRow = CategoryRow;
		}

		ItemsList.Add(CategoryRow);
	}

	FilteredItemsList = ItemsList;

	TotalItemsCount = NumItems;
	FilteredItemsCount = NumItems;
}

TArray<FString> SVoxelGraphParameterSelector::GetCategoryChain(const FString& Category) const
{
	TArray<FString> Categories;
	FEditorCategoryUtils::GetCategoryDisplayString(Category).ParseIntoArray(Categories, TEXT("|"), true);
	return Categories;
}