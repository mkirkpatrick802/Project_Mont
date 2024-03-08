// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "VoxelTerminalGraph.h"
#include "VoxelParameterOverridesDetails.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"

VOXEL_CUSTOMIZE_CLASS(UVoxelTerminalGraph)(IDetailLayoutBuilder& DetailLayout)
{
	UVoxelTerminalGraph* TerminalGraph = GetUniqueObjectBeingCustomized(DetailLayout);
	if (!ensure(TerminalGraph))
	{
		return;
	}

	if (IDetailsView* DetailView = DetailLayout.GetDetailsView())
	{
		if (DetailView->GetGenericLayoutDetailsDelegate().IsBound())
		{
			// Manual customization, exit
			return;
		}
	}

	const TWeakObjectPtr<UVoxelTerminalGraph> WeakTerminalGraph = MakeWeakObjectPtr(TerminalGraph);

	// Hide ExposeToLibrary
	{
		const TSharedRef<IPropertyHandle> ExposeToLibraryHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelTerminalGraph, bExposeToLibrary));
		if (!TerminalGraph->GetGraph().IsFunctionLibrary())
		{
			ExposeToLibraryHandle->MarkHiddenByCustomization();
		}
	}

	if (TerminalGraph->IsMainTerminalGraph())
	{
		const TWeakObjectPtr<UVoxelGraph> WeakGraph = &TerminalGraph->GetGraph();

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
			SNew(SObjectPropertyEntryBox)
			.AllowedClass(UVoxelGraph::StaticClass())
			.AllowClear(true)
			.ThumbnailPool(FVoxelEditorUtilities::GetThumbnailPool())
			.ObjectPath_Lambda([=]() -> FString
			{
				const UVoxelGraph* Graph = WeakGraph.Get();
				if (!ensureVoxelSlow(Graph))
				{
					return {};
				}

				const UVoxelGraph* BaseGraph = Graph->GetBaseGraph_Unsafe();
				if (!BaseGraph)
				{
					return {};
				}

				return BaseGraph->GetPathName();
			})
			.OnObjectChanged_Lambda([=](const FAssetData& NewAssetData)
			{
				UVoxelGraph* Graph = WeakGraph.Get();
				if (!ensureVoxelSlow(Graph))
				{
					return;
				}

				Graph->PreEditChange(nullptr);
				Graph->SetBaseGraph(CastEnsured<UVoxelGraph>(NewAssetData.GetAsset()));
				Graph->PostEditChange();
			})
		];

		Category.AddProperty(DetailLayout.AddObjectPropertyData({ WeakGraph.Get() }, GET_MEMBER_NAME_STATIC(UVoxelGraph, Category)));
		Category.AddProperty(DetailLayout.AddObjectPropertyData({ WeakGraph.Get() }, GET_MEMBER_NAME_STATIC(UVoxelGraph, Description)));
		Category.AddProperty(DetailLayout.AddObjectPropertyData({ WeakGraph.Get() }, GET_MEMBER_NAME_STATIC(UVoxelGraph, DisplayNameOverride)));
		Category.AddProperty(DetailLayout.AddObjectPropertyData({ WeakGraph.Get() }, GET_MEMBER_NAME_STATIC(UVoxelGraph, bEnableThumbnail)));
		Category.AddProperty(DetailLayout.AddObjectPropertyData({ WeakGraph.Get() }, GET_MEMBER_NAME_STATIC(UVoxelGraph, bUseAsTemplate)));
		Category.AddProperty(DetailLayout.AddObjectPropertyData({ WeakGraph.Get() }, GET_MEMBER_NAME_STATIC(UVoxelGraph, bShowInContextMenu)));

		KeepAlive(FVoxelParameterOverridesDetails::Create(
			DetailLayout,
			{ &TerminalGraph->GetGraph() },
			FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout)));

		return;
	}

	IDetailCategoryBuilder& FunctionCategory = DetailLayout.EditCategory("Function");

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	FunctionCategory.AddCustomRow(INVTEXT("Name"))
	.IsEnabled(TerminalGraph->IsTopmostTerminalGraph())
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(INVTEXT("Name"))
	]
	.ValueContent()
	[
		SNew(SBox)
		.MinDesiredWidth(125.f)
		[
			SNew(SEditableTextBox)
			.Font(FVoxelEditorUtilities::Font())
			.Text_Lambda([=]() -> FText
			{
				if (!ensure(WeakTerminalGraph.IsValid()))
				{
					return {};
				}
				return FText::FromString(WeakTerminalGraph->GetDisplayName());
			})
			.OnTextCommitted_Lambda([=](const FText& NewText, ETextCommit::Type)
			{
				if (!ensure(WeakTerminalGraph.IsValid()))
				{
					return;
				}

				WeakTerminalGraph->PreEditChange(nullptr);
				WeakTerminalGraph->UpdateMetadata([&](FVoxelGraphMetadata& Metadata)
				{
					Metadata.DisplayName = NewText.ToString();
				});
				WeakTerminalGraph->PostEditChange();
			})
		]
	];

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	FunctionCategory.AddCustomRow(INVTEXT("Category"))
	.IsEnabled(TerminalGraph->IsTopmostTerminalGraph())
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(INVTEXT("Category"))
	]
	.ValueContent()
	[
		SNew(SBox)
		.MinDesiredWidth(125.f)
		[
			SNew(SVoxelDetailComboBox<FString>)
			.RefreshDelegate(this, DetailLayout)
			.Options_Lambda([WeakGraph = MakeWeakObjectPtr(TerminalGraph->GetGraph())]() -> TArray<FString>
			{
				const UVoxelGraph* Graph = WeakGraph.Get();
				if (!ensure(Graph))
				{
					return {};
				}

				TSet<FString> Categories;
				for (const FGuid& Guid : Graph->GetTerminalGraphs())
				{
					if (Guid == GVoxelMainTerminalGraphGuid)
					{
						continue;
					}

					Categories.Add(Graph->FindTerminalGraphChecked(Guid).GetMetadata().Category);
				}

				Categories.Remove("");
				Categories.Add("Default");
				return Categories.Array();
			})
			.CurrentOption_Lambda([=]() -> FString
			{
				if (!ensure(WeakTerminalGraph.IsValid()))
				{
					return {};
				}

				const FString Category = WeakTerminalGraph->GetMetadata().Category;
				if (Category.IsEmpty())
				{
					return "Default";
				}
				return Category;
			})
			.CanEnterCustomOption(true)
			.OptionText(MakeLambdaDelegate([](const FString Option)
			{
				return Option;
			}))
			.OnSelection_Lambda([=](const FString NewValue)
			{
				if (!ensure(WeakTerminalGraph.IsValid()))
				{
					return;
				}

				WeakTerminalGraph->PreEditChange(nullptr);
				WeakTerminalGraph->UpdateMetadata([&](FVoxelGraphMetadata& Metadata)
				{
					if (NewValue == "Default")
					{
						Metadata.Category = {};
					}
					else
					{
						Metadata.Category = NewValue;
					}
				});
				WeakTerminalGraph->PostEditChange();
			})
		]
	];

	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////

	FunctionCategory.AddCustomRow(INVTEXT("Description"))
	.IsEnabled(TerminalGraph->IsTopmostTerminalGraph())
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(INVTEXT("Description"))
	]
	.ValueContent()
	[
		SNew(SBox)
		.MinDesiredWidth(125.f)
		[
			SNew(SMultiLineEditableTextBox)
			.Font(FVoxelEditorUtilities::Font())
			.Text_Lambda([=]() -> FText
			{
				if (!ensure(WeakTerminalGraph.IsValid()))
				{
					return {};
				}
				return FText::FromString(WeakTerminalGraph->GetMetadata().Description);
			})
			.OnTextCommitted_Lambda([=](const FText& NewText, ETextCommit::Type)
			{
				if (!ensure(WeakTerminalGraph.IsValid()))
				{
					return;
				}

				WeakTerminalGraph->PreEditChange(nullptr);
				WeakTerminalGraph->UpdateMetadata([&](FVoxelGraphMetadata& Metadata)
				{
					Metadata.Description = NewText.ToString();
				});
				WeakTerminalGraph->PostEditChange();
			})
		]
	];
}