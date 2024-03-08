// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SItemSelector.h"
#include "SVoxelNewAssetInstanceFilterBox.h"

class SVoxelNewAssetInstancePickerList : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnTemplateAssetActivated, const FAssetData&);
	DECLARE_DELEGATE_RetVal_OneParam(FString, FGetAssetText, const FAssetData&)
	DECLARE_DELEGATE_RetVal_OneParam(bool, FShowAsset, const FAssetData&)

public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_EVENT(FOnTemplateAssetActivated, OnTemplateAssetActivated);
		SLATE_EVENT(FGetAssetText, OnGetAssetCategory)
		SLATE_EVENT(FGetAssetText, OnGetAssetDescription)
		SLATE_EVENT(FShowAsset, ShowAsset)
	};

	void Construct(const FArguments& InArgs, UClass* AssetClass);

	TArray<FAssetData> GetSelectedAssets() const;

private:
	TArray<FText> OnGetCategoriesForItem(const FAssetData& Item) const;
	TArray<FText> OnGetSectionsForItem(const FAssetData& Item) const;
	TSharedRef<SWidget> OnGenerateWidgetForCategory(const FText& Category) const;
	TSharedRef<SWidget> OnGenerateWidgetForSection(const FText& Section) const;
	TSharedRef<SWidget> OnGenerateWidgetForItem(const FAssetData& Item);

private:
	TArray<FAssetData> GetAssetDataForSelector(const UClass* AssetClass) const;
	void TriggerRefresh(const TMap<EVoxelGraphScriptSource, bool>& SourceState) const;

	static FString GetAssetName(const FAssetData& Item);
	static int32 SectionNameToValue(const FText& Category);

private:
	TSharedPtr<SVoxelNewAssetInstanceFilterBox> FilterBox;

	typedef SItemSelector<FText, FAssetData> SAssetItemSelector;
	TSharedPtr<SAssetItemSelector> ItemSelector;

	FOnTemplateAssetActivated OnTemplateAssetActivatedDelegate;
	FGetAssetText OnGetAssetCategoryDelegate;
	FGetAssetText OnGetAssetDescriptionDelegate;
	FShowAsset ShowAssetDelegate;

	static FText ProjectCategory;
	static FText VoxelExamplesCategory;
	static FText UncategorizedCategory;
};