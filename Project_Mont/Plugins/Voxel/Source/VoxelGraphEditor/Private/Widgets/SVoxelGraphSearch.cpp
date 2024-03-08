// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphSearch.h"
#include "VoxelGraph.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphSearchItem.h"
#include "VoxelGraphSearchManager.h"

void FVoxelGraphSearchSettings::LoadFromIni()
{
#define Load(Variable) GConfig->GetBool(TEXT("VoxelGraphSearch"), TEXT(#Variable), Variable, GEditorPerProjectIni);
	Load(bSearchNodes);
	Load(bSearchPins);
	Load(bSearchParameters);
	Load(bSearchTypes);
	Load(bSearchDescriptions);
	Load(bSearchDefaultValues);
	Load(bRemoveSpaces);
#undef Load
}

bool FVoxelGraphSearchSettings::ShowTag(const EVoxelGraphSearchResultTag Tag) const
{
	switch (Tag)
	{
	default: ensure(false);
	case EVoxelGraphSearchResultTag::Special: return true;
	case EVoxelGraphSearchResultTag::Graph: return true;
	case EVoxelGraphSearchResultTag::TerminalGraph: return true;
	case EVoxelGraphSearchResultTag::Node: return bSearchNodes || bSearchPins;
	case EVoxelGraphSearchResultTag::NodeTitle: return bSearchNodes;
	case EVoxelGraphSearchResultTag::Pin: return bSearchPins;
	case EVoxelGraphSearchResultTag::Parameter: return bSearchParameters;
	case EVoxelGraphSearchResultTag::Input: return bSearchParameters;
	case EVoxelGraphSearchResultTag::Output: return bSearchParameters;
	case EVoxelGraphSearchResultTag::LocalVariable: return bSearchParameters;
	case EVoxelGraphSearchResultTag::Type: return bSearchTypes;
	case EVoxelGraphSearchResultTag::Description: return bSearchDescriptions;
	case EVoxelGraphSearchResultTag::DefaultValue: return bSearchDefaultValues;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphSearch::Construct(const FArguments& InArgs)
{
	WeakToolkit = InArgs._Toolkit;
	bIsGlobalSearch = InArgs._IsGlobalSearch;

	SearchSettings.LoadFromIni();

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f, 5.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SAssignNew(SearchTextField, SSearchBox)
					.HintText(INVTEXT("Enter function or event name to find references..."))
					.OnTextCommitted_Lambda([=](const FText& Text, const ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter)
						{
							SearchValue = Text.ToString();
							MakeSearchQuery(SearchValue);
						}
					})
					.DelayChangeNotificationsWhileTyping(false)
				]
				+ SHorizontalBox::Slot()
				.Padding(4.f, 0.f, 2.f, 0.f)
				.AutoWidth()
				[
					SNew(SButton)
					.Visibility(bIsGlobalSearch ? EVisibility::Collapsed : EVisibility::Visible)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([=]
					{
						if (const TSharedPtr<SVoxelGraphSearch> GlobalSearch = GVoxelGraphSearchManager->OpenGlobalSearch())
						{
							GlobalSearch->SearchSettings = SearchSettings;
							GlobalSearch->FocusForUse(SearchValue);
						}

						return FReply::Handled();
					})
					.ToolTipText(INVTEXT("Find in all Voxel Graphs"))
					[
						SNew(STextBlock)
						.ColorAndOpacity(FSlateColor::UseForeground())
						.TextStyle(FAppStyle::Get(), "FindResults.FindInBlueprints")
						.Text(FText::FromString(TEXT("\xf1e5")))
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Brushes.Recessed"))
					.Padding(8.f, 8.f, 4.f, 0.f)
					[
						SAssignNew(TreeView, STreeViewType)
						.ItemHeight(24.f)
						.TreeItemsSource(&RootItems)
						.OnGenerateRow(this, &SVoxelGraphSearch::OnGenerateRow)
						.OnGetChildren_Lambda([=](const TSharedPtr<FVoxelGraphSearchItem> Item, TArray<TSharedPtr<FVoxelGraphSearchItem>>& OutChildren)
						{
							if (!ensure(Item))
							{
								return;
							}

							OutChildren.Append(Item->GetChildren());
						})
						.OnMouseButtonDoubleClick_Lambda([=](const TSharedPtr<FVoxelGraphSearchItem> Item)
						{
							if (!ensure(Item))
							{
								return;
							}

							Item->OnClick();
						})
						.SelectionMode(ESelectionMode::Multi)
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					CreateSearchSettingsWidget()
				]
			]
		]
	];
}

void SVoxelGraphSearch::FocusForUse(const FString& NewSearchTerms)
{
	if (!bIsGlobalSearch)
	{
		const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();
		if (!ensure(Toolkit))
		{
			return;
		}

		const TSharedPtr<FTabManager> TabManager = Toolkit->GetTabManager();
		if (!ensure(TabManager))
		{
			return;
		}

		if (const TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(FTabId(FVoxelGraphToolkit::SearchTabId)))
		{
			TabManager->DrawAttention(Tab.ToSharedRef());
		}
		else
		{
			TabManager->TryInvokeTab(FTabId(FVoxelGraphToolkit::SearchTabId));
		}
	}

	FWidgetPath SearchTextBoxPath;
	if (FSlateApplication::Get().GeneratePathToWidgetUnchecked(SearchTextField.ToSharedRef(), SearchTextBoxPath))
	{
		FSlateApplication::Get().SetKeyboardFocus(SearchTextBoxPath, EFocusCause::SetDirectly);
	}

	if (NewSearchTerms.IsEmpty())
	{
		return;
	}

	SearchTextField->SetText(FText::FromString(NewSearchTerms));
	SearchValue = NewSearchTerms;
	MakeSearchQuery(SearchValue);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphSearch::CreateSearchSettingsWidget()
{
	FMenuBuilder MenuBuilder(true, nullptr);

#define Add(Label, ToolTip, Variable) \
	MenuBuilder.AddMenuEntry( \
		INVTEXT(Label), \
		INVTEXT(ToolTip), \
		FSlateIcon(), \
		FUIAction(FExecuteAction::CreateLambda([this] \
		{ \
			SearchSettings.Variable = !SearchSettings.Variable; \
			GConfig->SetBool(TEXT("VoxelGraphSearch"), TEXT(#Variable), SearchSettings.Variable, GEditorPerProjectIni); \
			MakeSearchQuery(SearchValue); \
		}), \
		FCanExecuteAction(), \
		FGetActionCheckState::CreateLambda([this] \
		{ \
			return SearchSettings.Variable ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; \
		})), \
		{}, \
		EUserInterfaceActionType::ToggleButton);

	MenuBuilder.BeginSection("", INVTEXT("Show in Search Results"));
	{
		Add("Nodes", "Search through nodes", bSearchNodes);
		Add("Pins", "Search through node pins", bSearchPins);
		Add("Parameters", "Search through the graph parameter list", bSearchParameters);
		Add("Type", "Compare the search term to parameter/pin types in lookup", bSearchTypes);
		Add("Description", "Search node/parameter/pin descriptions", bSearchDescriptions);
		Add("Default Value", "Compare the search term parameter/pin default values in lookup", bSearchDefaultValues);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("", INVTEXT("Misc"));
	{
		Add("Remove spaces in Search Results", "Will remove spaces from search lookup string, which will make search results more lenient for partial matches", bRemoveSpaces);
	}
	MenuBuilder.EndSection();

#undef Add

	return MenuBuilder.MakeWidget();
}

TSharedRef<ITableRow> SVoxelGraphSearch::OnGenerateRow(TSharedPtr<FVoxelGraphSearchItem> Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	TSharedRef<SWidget> IconWidget = SNullWidget::NullWidget;
	{
		FSlateColor IconColor = FLinearColor::White;
		if (const FSlateBrush* Icon = Item->GetIcon(IconColor))
		{
			IconWidget =
				SNew(SImage)
				.Image(Icon)
				.ColorAndOpacity(IconColor);
		}
	}

	return
		SNew(STableRow<TSharedPtr<FVoxelGraphSearchItem>>, OwnerTable)
		.Style(&FAppStyle::GetWidgetStyle<FTableRowStyle>("ShowParentsTableView.Row"))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				IconWidget
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			.Padding(2.f)
			[
				SNew(SVoxelDetailText)
				.Text(FText::FromString(Item->GetName().Replace(TEXT("\n"), TEXT(" "))))
				.HighlightText(HighlightText)
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.FillWidth(1.f)
			.Padding(2.f)
			[
				SNew(SVoxelDetailText)
				.Text(FText::FromString(Item->GetComment()))
				.HighlightText(HighlightText)
			]
		];
}


void SVoxelGraphSearch::MakeSearchQuery(const FString& SearchQuery)
{
	TArray<FString> Tokens;
	if (SearchQuery.Contains("\"") &&
		SearchQuery.ParseIntoArray(Tokens, TEXT("\""), true) > 0)
	{
		for (FString& Token : Tokens)
		{
			Token = Token.TrimQuotes().Replace(TEXT(" "), TEXT(""));
		}

		Tokens.RemoveAll([](const FString& Token)
		{
			return Token.IsEmpty();
		});
	}
	else
	{
		SearchQuery.ParseIntoArray(Tokens, TEXT(" "), true);
	}

	RootItems.Reset();

	const TSharedRef<FVoxelGraphSearchItem::FContext> Context = MakeVoxelShared<FVoxelGraphSearchItem::FContext>();
	Context->Tokens = Tokens;
	Context->Settings = SearchSettings;

	if (Tokens.Num() > 0)
	{
		HighlightText = FText::FromString(SearchValue);

		if (bIsGlobalSearch)
		{
			UVoxelGraph::LoadAllGraphs();

			ForEachObjectOfClass_Copy<UVoxelGraph>([&](UVoxelGraph& Graph)
			{
				const TSharedRef<FVoxelGraphSearchItem_Graph> Item = MakeVoxelShared<FVoxelGraphSearchItem_Graph>(&Graph);
				Item->Initialize(Context);

				if (Item->GetChildren().Num() > 0)
				{
					RootItems.Add(Item);
				}
			});
		}
		else
		{
			const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();
			if (!ensure(Toolkit))
			{
				return;
			}

			const TSharedRef<FVoxelGraphSearchItem_Graph> Item = MakeVoxelShared<FVoxelGraphSearchItem_Graph>(Toolkit->Asset);
			Item->Initialize(Context);
			RootItems.Add(Item);
		}
	}

	if (RootItems.Num() == 0)
	{
		RootItems.Add(MakeVoxelShared<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Special, "", "No Results found"));
	}

	TreeView->RequestTreeRefresh();

	TSet<TSharedPtr<FVoxelGraphSearchItem>> AllItems;
	TArray<TSharedPtr<FVoxelGraphSearchItem>> QueuedItems = RootItems;
	while (QueuedItems.Num() > 0)
	{
		const TSharedPtr<FVoxelGraphSearchItem> Item = QueuedItems.Pop(false);
		if (AllItems.Contains(Item))
		{
			continue;
		}

		AllItems.Add(Item);
		QueuedItems.Append(Item->GetChildren());
	}

	for (const TSharedPtr<FVoxelGraphSearchItem>& Item : AllItems)
	{
		TreeView->SetItemExpansion(Item, true);
	}
}