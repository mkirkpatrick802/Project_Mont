// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Widgets/SVoxelPinTypeSelector.h"
#include "VoxelSurface.h"
#include "VoxelGraphVisuals.h"
#include "Point/VoxelPointSet.h"
#include "Channel/VoxelChannelName.h"
#include "SListViewSelectorDropdownMenu.h"

VOXEL_INITIALIZE_STYLE(GraphPinTypeSelectorEditor)
{
	Set("Pill.Buffer", new IMAGE_BRUSH_SVG("Graphs/BufferPill", CoreStyleConstants::Icon16x16));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelPinTypeSelector::Construct(const FArguments& InArgs)
{
	AllowedTypes = InArgs._AllowedTypes;
	bAllowWildcard = InArgs._AllowWildcard;

	OnTypeChanged = InArgs._OnTypeChanged;
	ensure(OnTypeChanged.IsBound());
	OnCloseMenu = InArgs._OnCloseMenu;

	FillTypesList();

	SAssignNew(TypeTreeView, SVoxelPinTypeTreeView)
	.TreeItemsSource(&FilteredTypesList)
	.SelectionMode(ESelectionMode::Single)
	.OnGenerateRow(this, &SVoxelPinTypeSelector::GenerateTypeTreeRow)
	.OnSelectionChanged(this, &SVoxelPinTypeSelector::OnTypeSelectionChanged)
	.OnGetChildren(this, &SVoxelPinTypeSelector::GetTypeChildren);

	SAssignNew(FilterTextBox, SSearchBox)
	.OnTextChanged(this, &SVoxelPinTypeSelector::OnFilterTextChanged)
	.OnTextCommitted(this, &SVoxelPinTypeSelector::OnFilterTextCommitted);

	ChildSlot
	[
		SNew(SListViewSelectorDropdownMenu<TSharedPtr<FVoxelPinTypeSelectorRow>>, FilterTextBox, TypeTreeView)
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
					TypeTreeView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f, 0.f, 8.f, 4.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]
				{
					const FString ItemText = FilteredTypesCount == 1 ? " item" : " items";
					return FText::FromString(FText::AsNumber(FilteredTypesCount).ToString() + ItemText);
				})
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelPinTypeSelector::ClearSelection() const
{
	TypeTreeView->SetSelection(nullptr, ESelectInfo::OnNavigation);
	TypeTreeView->ClearExpandedItems();
}

TSharedPtr<SWidget> SVoxelPinTypeSelector::GetWidgetToFocus() const
{
	return FilterTextBox;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<ITableRow> SVoxelPinTypeSelector::GenerateTypeTreeRow(
	const TSharedPtr<FVoxelPinTypeSelectorRow> RowItem,
	const TSharedRef<STableViewBase>& OwnerTable) const
{
	const FLinearColor Color = RowItem->Children.Num() > 0 ? FLinearColor::White : GetColor(RowItem->Type);
	const FSlateBrush* Image = RowItem->Children.Num() > 0 ? FAppStyle::GetBrush("NoBrush") : GetIcon(RowItem->Type.GetInnerType());

	return
		SNew(STableRow<TSharedPtr<FVoxelPinTypeSelectorRow>>, OwnerTable)
		[
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
				.Font(RowItem->Children.Num() > 0 ? FAppStyle::GetFontStyle(TEXT("Kismet.TypePicker.CategoryFont")) : FAppStyle::GetFontStyle(TEXT("Kismet.TypePicker.NormalFont")))
			]
		];
}

void SVoxelPinTypeSelector::OnTypeSelectionChanged(
	const TSharedPtr<FVoxelPinTypeSelectorRow> Selection,
	const ESelectInfo::Type SelectInfo) const
{
	if (SelectInfo == ESelectInfo::OnNavigation)
	{
		if (TypeTreeView->WidgetFromItem(Selection).IsValid())
		{
			OnCloseMenu.ExecuteIfBound();
		}

		return;
	}

	if (!Selection)
	{
		return;
	}

	if (Selection->Children.Num() == 0)
	{
		OnCloseMenu.ExecuteIfBound();
		OnTypeChanged.ExecuteIfBound(Selection->Type);
		return;
	}

	TypeTreeView->SetItemExpansion(Selection, !TypeTreeView->IsItemExpanded(Selection));

	if (SelectInfo == ESelectInfo::OnMouseClick)
	{
		TypeTreeView->ClearSelection();
	}
}

void SVoxelPinTypeSelector::GetTypeChildren(
	const TSharedPtr<FVoxelPinTypeSelectorRow> PinTypeRow,
	TArray<TSharedPtr<FVoxelPinTypeSelectorRow>>& PinTypeRows) const
{
	PinTypeRows = PinTypeRow->Children;
}

void SVoxelPinTypeSelector::OnFilterTextChanged(const FText& NewText)
{
	SearchText = NewText;
	FilteredTypesList = {};

	GetChildrenMatchingSearch(NewText);
	TypeTreeView->RequestTreeRefresh();
}

void SVoxelPinTypeSelector::OnFilterTextCommitted(const FText& NewText, const ETextCommit::Type CommitInfo) const
{
	if (CommitInfo != ETextCommit::OnEnter)
	{
		return;
	}

	TArray<TSharedPtr<FVoxelPinTypeSelectorRow>> SelectedItems = TypeTreeView->GetSelectedItems();
	if (SelectedItems.Num() > 0)
	{
		TypeTreeView->SetSelection(SelectedItems[0]);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelPinTypeSelector::GetChildrenMatchingSearch(const FText& InSearchText)
{
	FilteredTypesCount = 0;

	TArray<FString> FilterTerms;
	TArray<FString> SanitizedFilterTerms;

	if (InSearchText.IsEmpty())
	{
		FilteredTypesList = TypesList;
		FilteredTypesCount = TotalTypesCount;
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

	const auto SearchMatches = [&FilterTerms, &SanitizedFilterTerms](const TSharedPtr<FVoxelPinTypeSelectorRow>& TypeRow) -> bool
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

	const auto LookThroughList = [&](const TArray<TSharedPtr<FVoxelPinTypeSelectorRow>>& UnfilteredList, TArray<TSharedPtr<FVoxelPinTypeSelectorRow>>& OutFilteredList, auto& Lambda) -> bool
	{
		bool bReturnVal = false;
		for (const TSharedPtr<FVoxelPinTypeSelectorRow>& TypeRow : UnfilteredList)
		{
			const bool bMatchesItem = SearchMatches(TypeRow);
			if (TypeRow->Children.Num() == 0 ||
				bMatchesItem)
			{
				if (bMatchesItem)
				{
					OutFilteredList.Add(TypeRow);

					if (TypeRow->Children.Num() > 0 &&
						TypeTreeView.IsValid())
					{
						TypeTreeView->SetItemExpansion(TypeRow, true);
					}

					FilteredTypesCount += FMath::Max(1, TypeRow->Children.Num());

					bReturnVal = true;
				}
				continue;
			}

			TArray<TSharedPtr<FVoxelPinTypeSelectorRow>> ValidChildren;
			if (Lambda(TypeRow->Children, ValidChildren, Lambda))
			{
				TSharedRef<FVoxelPinTypeSelectorRow> NewCategory = MakeVoxelShared<FVoxelPinTypeSelectorRow>(TypeRow->Name, ValidChildren);
				OutFilteredList.Add(NewCategory);

				if (TypeTreeView.IsValid())
				{
					TypeTreeView->SetItemExpansion(NewCategory, true);
				}

				bReturnVal = true;
			}
		}

		return bReturnVal;
	};

	LookThroughList(TypesList, FilteredTypesList, LookThroughList);
}

void SVoxelPinTypeSelector::FillTypesList()
{
	TypesList = {};

	TMap<FString, TSharedPtr<FVoxelPinTypeSelectorRow>> Categories;

	bool bCanSelectUniforms = false;
	for (const FVoxelPinType& Type : AllowedTypes.Get().GetTypes())
	{
		if (!Type.IsBuffer())
		{
			bCanSelectUniforms = true;
			break;
		}
	}

	if (bAllowWildcard)
	{
		TypesList.Add(MakeVoxelShared<FVoxelPinTypeSelectorRow>(FVoxelPinType::Make<FVoxelWildcard>()));
	}

	int32 NumTypes = 0;
	for (const FVoxelPinType& Type : AllowedTypes.Get().GetTypes())
	{
		if (Type.IsBuffer() &&
			bCanSelectUniforms)
		{
			continue;
		}
		NumTypes++;

		// Only display inner type, uniform/buffer selection is done in dropdown
		const FVoxelPinType ExposedType = Type.GetInnerExposedType();

		FString TargetCategory = "Default";

		if (ExposedType.Is<uint8>() &&
			ExposedType.GetEnum())
		{
			TargetCategory = "Enums";
		}
		else if (ExposedType.IsClass())
		{
			TargetCategory = "Classes";
		}
		else if (ExposedType.IsObject())
		{
			TargetCategory = "Objects";
		}
		else if (ExposedType.IsStruct())
		{
			const UScriptStruct* Struct = ExposedType.GetStruct();
			if (Struct->HasMetaData("TypeCategory"))
			{
				TargetCategory = Struct->GetMetaData("TypeCategory");
			}
			else if (
				!ExposedType.Is<FVector2D>() &&
				!ExposedType.Is<FVector>() &&
				!ExposedType.Is<FLinearColor>() &&
				!ExposedType.Is<FIntPoint>() &&
				!ExposedType.Is<FIntVector>() &&
				!ExposedType.Is<FIntVector4>() &&
				!ExposedType.Is<FQuat>() &&
				!ExposedType.Is<FRotator>() &&
				!ExposedType.Is<FTransform>() &&
				!ExposedType.Is<FVoxelSurface>() &&
				!ExposedType.Is<FVoxelPointSet>() &&
				!ExposedType.Is<FVoxelChannelName>())
			{
				TargetCategory = "Structs";
			}
		}

		if (TargetCategory.IsEmpty() ||
			TargetCategory == "Default")
		{
			TypesList.Add(MakeVoxelShared<FVoxelPinTypeSelectorRow>(Type));
			continue;
		}

		if (!Categories.Contains(TargetCategory))
		{
			Categories.Add(TargetCategory, MakeVoxelShared<FVoxelPinTypeSelectorRow>(TargetCategory));
		}

		Categories[TargetCategory]->Children.Add(MakeVoxelShared<FVoxelPinTypeSelectorRow>(Type));
	}

	for (const auto& It : Categories)
	{
		TypesList.Add(It.Value);
	}
	FilteredTypesList = TypesList;

	TotalTypesCount = NumTypes;
	FilteredTypesCount = NumTypes;
}

const FSlateBrush* SVoxelPinTypeSelector::GetIcon(const FVoxelPinType& PinType) const
{
	return FVoxelGraphVisuals::GetPinIcon(PinType).GetIcon();
}

FLinearColor SVoxelPinTypeSelector::GetColor(const FVoxelPinType& PinType) const
{
	return FVoxelGraphVisuals::GetPinColor(PinType);
}