// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "VoxelParameterOverridesDetails.h"

VOXEL_CUSTOMIZE_CLASS(UVoxelGraph)(IDetailLayoutBuilder& DetailLayout)
{
	FVoxelEditorUtilities::EnableRealtime();

	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraph, Category))->MarkHiddenByCustomization();
	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraph, Description))->MarkHiddenByCustomization();
	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraph, DisplayNameOverride))->MarkHiddenByCustomization();
	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraph, bEnableThumbnail))->MarkHiddenByCustomization();
	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraph, bUseAsTemplate))->MarkHiddenByCustomization();
	DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelGraph, bShowInContextMenu))->MarkHiddenByCustomization();

	if (IDetailsView* DetailView = DetailLayout.GetDetailsView())
	{
		if (DetailView->GetGenericLayoutDetailsDelegate().IsBound())
		{
			// Manual customization, exit
			return;
		}
	}

	const TVoxelArray<TWeakObjectPtr<UVoxelGraph>> WeakGraphs = GetWeakObjectsBeingCustomized(DetailLayout);

	const auto GetBaseGraphs = [=]
	{
		TVoxelSet<UVoxelGraph*> BaseGraphs;
		for (const TWeakObjectPtr<UVoxelGraph>& WeakGraph : WeakGraphs)
		{
			const UVoxelGraph* Graph = WeakGraph.Get();
			if (!ensureVoxelSlow(Graph))
			{
				continue;
			}

			BaseGraphs.Add(Graph->GetBaseGraph_Unsafe());
		}
		return BaseGraphs;
	};

	// Force graph at the top
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory(
		"Config",
		{},
		ECategoryPriority::Important);

	Category.AddCustomRow(INVTEXT("BaseGraph"))
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(INVTEXT("Base Graph"))
	]
	.ValueContent()
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SVoxelDetailText)
			.Text(INVTEXT("Multiple Values"))
			.Visibility_Lambda([=]
			{
				return GetBaseGraphs().Num() != 1 ? EVisibility::Visible : EVisibility::Collapsed;
			})
		]
		+ SOverlay::Slot()
		[
			SNew(SObjectPropertyEntryBox)
			.Visibility_Lambda([=]
			{
				return GetBaseGraphs().Num() == 1 ? EVisibility::Visible : EVisibility::Collapsed;
			})
			.AllowedClass(UVoxelGraph::StaticClass())
			.AllowClear(true)
			.ThumbnailPool(FVoxelEditorUtilities::GetThumbnailPool())
			.ObjectPath_Lambda([=]() -> FString
			{
				const TVoxelSet<UVoxelGraph*> BaseGraphs = GetBaseGraphs();
				if (BaseGraphs.Num() != 1)
				{
					return {};
				}

				const UVoxelGraph* BaseGraph = BaseGraphs.GetUniqueValue();
				if (!BaseGraph)
				{
					return {};
				}

				return BaseGraph->GetPathName();
			})
			.OnObjectChanged_Lambda([=](const FAssetData& NewAssetData)
			{
				UVoxelGraph* NewBaseGraph = CastEnsured<UVoxelGraph>(NewAssetData.GetAsset());
				for (const TWeakObjectPtr<UVoxelGraph>& WeakGraph : WeakGraphs)
				{
					UVoxelGraph* Graph = WeakGraph.Get();
					if (!ensure(Graph))
					{
						continue;
					}

					Graph->PreEditChange(nullptr);
					Graph->SetBaseGraph(NewBaseGraph);
					Graph->PostEditChange();
				}
			})
		]
	];

	KeepAlive(FVoxelParameterOverridesDetails::Create(
		DetailLayout,
		GetObjectsBeingCustomized(DetailLayout),
		FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout)));
}