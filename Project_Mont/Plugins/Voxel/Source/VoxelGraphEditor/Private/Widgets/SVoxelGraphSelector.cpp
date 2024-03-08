// Fill out your copyright notice in the Description page of Project Settings.

#include "SVoxelGraphSelector.h"
#include "VoxelGraph.h"
#include "VoxelGraphTracker.h"
#include "Subsystems/AssetEditorSubsystem.h"

void SVoxelGraphSelector::Construct(const FArguments& InArgs)
{
	OnGraphSelected = InArgs._OnGraphSelected;
	IsGraphValid = InArgs._IsGraphValid;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(FName("AssetRegistry")).Get();
	AssetRegistry.OnAssetAdded().Add(MakeTSWeakPtrDelegate(this, [this](const FAssetData&)
	{
		RebuildList();
	}));
	AssetRegistry.OnAssetRemoved().Add(MakeTSWeakPtrDelegate(this, [this](const FAssetData&)
	{
		RebuildList();
	}));
	AssetRegistry.OnAssetRenamed().Add(MakeTSWeakPtrDelegate(this, [this](const FAssetData&, const FString&)
	{
		RebuildList();
	}));
	AssetRegistry.OnAssetUpdated().Add(MakeTSWeakPtrDelegate(this, [this](const FAssetData&)
	{
		RebuildList();
	}));

	// Rebuild whenever a node changed to see if we have a sculpt node now
	GVoxelGraphTracker->OnTerminalGraphTranslated(FVoxelGraphTracker::MakeAny<UVoxelTerminalGraph>()).Add(
		FOnVoxelGraphChanged::Make(this, [this]
		{
			RebuildList();
		}));

	FindAllGraphs();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.Padding(0.f, 2.f)
			.AutoHeight()
			[
				SNew(SSearchBox)
				.HintText(INVTEXT("Search..."))
				.OnTextChanged(this, &SVoxelGraphSelector::OnSearchChanged)
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(GraphsListView, SListView<TSharedPtr<FVoxelGraphListNode>>)
				.SelectionMode(ESelectionMode::Single)
				.ListItemsSource(&FilteredGraphsList)
				.OnGenerateRow(this, &SVoxelGraphSelector::OnGenerateRow)
				.OnSelectionChanged(MakeWeakPtrDelegate(this, [this](TSharedPtr<FVoxelGraphListNode> Node, const ESelectInfo::Type SelectType)
				{
					ActiveGraph = Node ? Cast<UVoxelGraph>(Node->Asset.GetAsset()) : nullptr;

					if (SelectType != ESelectInfo::Direct)
					{
						OnGraphSelected.ExecuteIfBound(ActiveGraph.Get());
					}
				}))
				.OnMouseButtonDoubleClick(MakeWeakPtrDelegate(this, [](TSharedPtr<FVoxelGraphListNode> Node)
				{
					if (!Node)
					{
						return;
					}

					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Node->Asset.GetSoftObjectPath());
				}))
				.ItemHeight(24.0f)
			]
		]
	];

	if (InArgs._SelectedGraph)
	{
		for (const TSharedPtr<FVoxelGraphListNode>& Node : FilteredGraphsList)
		{
			if (Node->WeakGraph == InArgs._SelectedGraph)
			{
				GraphsListView->SetSelection(Node);
				break;
			}
		}
	}
}

void SVoxelGraphSelector::SetSelectedGraph(const UVoxelGraph* Graph)
{
	if (!Graph)
	{
		return;
	}

	for (const TSharedPtr<FVoxelGraphListNode>& Node : FilteredGraphsList)
	{
		if (Node->WeakGraph == Graph)
		{
			GraphsListView->SetSelection(Node);
			break;
		}
	}
}

void SVoxelGraphSelector::OnSearchChanged(const FText& Text)
{
	LookupString = Text.ToString().ToLower();
	LookupText = Text;

	FilterGraphs();

	if (ActiveGraph.IsValid())
	{
		bool bContainsActiveGraph = false;
		for (const TSharedPtr<FVoxelGraphListNode>& Node : FilteredGraphsList)
		{
			if (Node->WeakGraph == ActiveGraph)
			{
				bContainsActiveGraph = true;
				break;
			}
		}

		if (!bContainsActiveGraph)
		{
			OnGraphSelected.ExecuteIfBound(nullptr);
		}
	}

	GraphsListView->RebuildList();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<ITableRow> SVoxelGraphSelector::OnGenerateRow(TSharedPtr<FVoxelGraphListNode> Node, const TSharedRef<STableViewBase>& OwningTable)
{
	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailConfig.bAllowFadeIn = false;

	return
		SNew(STableRow<TSharedPtr<FVoxelGraphListNode>>, OwningTable)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(7.5f, 1.5f, 5.f, 1.5f)
			[
				SNew(SBox)
				.WidthOverride(36.f)
				.HeightOverride(36.f)
				[
					Node->AssetThumbnail->MakeThumbnailWidget()
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.Padding(2.5f, 0.f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(Node->GetName())
					.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.AssetPickerBoldAssetNameText")
					.HighlightText_Lambda(MakeWeakPtrLambda(this, [this]
					{
						return LookupText;
					}))
				]
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Text(Node->GetDescription())
					.TextStyle(FVoxelEditorStyle::Get(), "Graph.NewAssetDialog.AssetPickerAssetNameText")
					.AutoWrapText(true)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(2.f)
			.VAlign(VAlign_Center)
			[
				PropertyCustomizationHelpers::MakeBrowseButton(MakeWeakPtrDelegate(this, [WeakNode = MakeWeakPtr(Node)]
				{
					const TSharedPtr<FVoxelGraphListNode> GraphNode = WeakNode.Pin();
					if (!GraphNode)
					{
						return;
					}

					FSlateApplication::Get().DismissAllMenus();
					TArray<FAssetData> Assets{ GraphNode->Asset };
					GEditor->SyncBrowserToObjects(Assets, true);
				}))
			]
		];
}

void SVoxelGraphSelector::FindAllGraphs()
{
	VOXEL_FUNCTION_COUNTER();

	GraphsList = {};

	UVoxelGraph::LoadAllGraphs();

	ForEachObjectOfClass<UVoxelGraph>([&](UVoxelGraph& Graph)
	{
		if (ensure(IsGraphValid.IsBound()) &&
			!IsGraphValid.Execute(Graph))
		{
			return;
		}

		GraphsList.Add(MakeShared<FVoxelGraphListNode>(Graph));
	});

	FilterGraphs();
}

void SVoxelGraphSelector::FilterGraphs()
{
	if (LookupString.IsEmpty())
	{
		FilteredGraphsList = GraphsList;
		return;
	}

	FilteredGraphsList = {};
	FilteredGraphsList.Reserve(GraphsList.Num());

	for (const TSharedPtr<FVoxelGraphListNode>& Graph : GraphsList)
	{
		if (Graph->LookupName.Contains(LookupString))
		{
			FilteredGraphsList.Add(Graph);
		}
	}
}

void SVoxelGraphSelector::RebuildList()
{
	VOXEL_FUNCTION_COUNTER();

	FindAllGraphs();

	if (!GraphsListView)
	{
		return;
	}

	GraphsListView->RebuildList();

	for (const TSharedPtr<FVoxelGraphListNode>& Node : FilteredGraphsList)
	{
		if (Node->WeakGraph == ActiveGraph)
		{
			GraphsListView->SetSelection(Node, ESelectInfo::Direct);
		}
	}
}