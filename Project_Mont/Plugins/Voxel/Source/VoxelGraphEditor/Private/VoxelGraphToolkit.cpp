// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphToolkit.h"
#include "VoxelEdGraph.h"
#include "VoxelGraphSchema.h"
#include "VoxelTerminalGraph.h"
#include "VoxelTerminalGraphRuntime.h"
#include "Widgets/SVoxelGraphSearch.h"
#include "Widgets/SVoxelGraphPreview.h"
#include "Widgets/SVoxelGraphMessages.h"
#include "Members/SVoxelGraphMembers.h"
#include "Customizations/VoxelGraphPreviewSettingsCustomization.h"
#include "SGraphPanel.h"
#include "SVoxelGraphEditorActionMenu.h"
#include "VoxelGraphEditorSummoner.h"

TSharedPtr<FVoxelGraphToolkit> FVoxelGraphToolkit::Get(const UEdGraph* EdGraph)
{
	if (!EdGraph)
	{
		return nullptr;
	}

	const UVoxelEdGraph* VoxelEdGraph = Cast<UVoxelEdGraph>(EdGraph);
	if (!ensure(VoxelEdGraph))
	{
		return nullptr;
	}

	return VoxelEdGraph->GetGraphToolkit();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphToolkit::Initialize()
{
	DocumentManager = MakeVoxelShared<FDocumentTracker>();
	// HostingApp is not used
	DocumentManager->Initialize(nullptr);
	DocumentManager->RegisterDocumentFactory(MakeVoxelShared<FVoxelGraphEditorSummoner>(SharedThis(this)));

	GraphPreview =
		SNew(SVoxelGraphPreview);

	GraphPreviewStats = GraphPreview->GetPreviewStats();

	GraphMembers =
		SNew(SVoxelGraphMembers)
		.Toolkit(SharedThis(this));

	SearchWidget =
		SNew(SVoxelGraphSearch)
		.Toolkit(SharedThis(this));

	GraphMessages =
		SNew(SVoxelGraphMessages)
		.Graph(Asset);

	{
		FDetailsViewArgs Args;
		Args.bAllowSearch = false;
		Args.bShowOptions = false;
		Args.bHideSelectionTip = true;
		Args.bShowPropertyMatrixButton = false;
		Args.NotifyHook = GetNotifyHook();
		Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		DetailsView = PropertyModule.CreateDetailView(Args);
	}

	{
		FDetailsViewArgs Args;
		Args.bAllowSearch = false;
		Args.bShowOptions = false;
		Args.bHideSelectionTip = true;
		Args.bShowPropertyMatrixButton = false;
		Args.NotifyHook = GetNotifyHook();
		Args.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Hide;

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PreviewDetailsView = PropertyModule.CreateDetailView(Args);
		PreviewDetailsView->SetGenericLayoutDetailsDelegate(MakeLambdaDelegate([]() -> TSharedRef<IDetailCustomization>
		{
			return MakeVoxelShared<FVoxelGraphPreviewSettingsCustomization>();
		}));
	}

	Selection = MakeVoxelShared<FVoxelGraphSelection>(
		*this,
		DetailsView.ToSharedRef(),
		GraphMembers.ToSharedRef());

	if (!Asset->IsFunctionLibrary())
	{
		Selection->SelectMainGraph();
	}

	CommandManager = MakeVoxelShared<FVoxelGraphCommandManager>(*this);
	CommandManager->MapActions();

	// Fixup graphs
	Asset->ForeachTerminalGraph_NoInheritance([&](UVoxelTerminalGraph& TerminalGraph)
	{
		FixupTerminalGraph(&TerminalGraph);
	});

	// Once everything is ready, update errors
	Asset->ForeachTerminalGraph_NoInheritance([&](const UVoxelTerminalGraph& TerminalGraph)
	{
		TerminalGraph.GetRuntime().EnsureIsCompiled();
	});

	// Force a message update if we're loading an already compiled graph
	GraphMessages->UpdateNodes();
}

void FVoxelGraphToolkit::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick();

	{
		VOXEL_SCOPE_COUNTER("NodesToReconstruct");

		const TSet<TWeakObjectPtr<UEdGraphNode>> NodesToReconstructCopy = MoveTemp(NodesToReconstruct);
		ensure(NodesToReconstruct.Num() == 0);

		for (const TWeakObjectPtr<UEdGraphNode>& WeakNode : NodesToReconstructCopy)
		{
			UEdGraphNode* Node = WeakNode.Get();
			if (!ensure(Node))
			{
				continue;
			}

			Node->ReconstructNode();
		}
	}

	if (bMessageUpdateQueued)
	{
		bMessageUpdateQueued = false;

		// Focus Messages tab
		const TSharedPtr<FTabManager> TabManager = GetTabManager();
		if (ensure(TabManager))
		{
			const TSharedPtr<SDockTab> MessagesTab = TabManager->FindExistingLiveTab(FTabId(MessagesTabId));
			if (ensure(MessagesTab) &&
				// Only focus if asset is focused
				TabManager->GetOwnerTab()->HasAnyUserFocus())
			{
				MessagesTab->DrawAttention();
			}
		}

		GraphMessages->UpdateNodes();
	}
}

void FVoxelGraphToolkit::BuildMenu(FMenuBarBuilder& MenuBarBuilder)
{
	Super::BuildMenu(MenuBarBuilder);

	MenuBarBuilder.AddPullDownMenu(
		INVTEXT("Graph"),
		INVTEXT(""),
		FNewMenuDelegate::CreateLambda([this](FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection("Search", INVTEXT("Search"));
			{
				MenuBuilder.AddMenuEntry(FVoxelGraphCommands::Get().FindInGraph);
				MenuBuilder.AddMenuEntry(FVoxelGraphCommands::Get().FindInGraphs);
			}
			MenuBuilder.EndSection();

			MenuBuilder.BeginSection("Tools", INVTEXT("Tools"));
			{
				MenuBuilder.AddMenuEntry(FVoxelGraphCommands::Get().ReconstructAllNodes);
			}
			MenuBuilder.EndSection();
		}));
}

void FVoxelGraphToolkit::BuildToolbar(FToolBarBuilder& ToolbarBuilder)
{
	Super::BuildToolbar(ToolbarBuilder);

	ToolbarBuilder.BeginSection("Voxel");
	ToolbarBuilder.AddToolBarButton(FVoxelGraphCommands::Get().Compile);
	ToolbarBuilder.AddToolBarButton(FVoxelGraphCommands::Get().CompileAll);
	ToolbarBuilder.AddToolBarButton(FVoxelGraphCommands::Get().EnableStats);
	ToolbarBuilder.AddToolBarButton(FVoxelGraphCommands::Get().EnableRangeStats);
	ToolbarBuilder.EndSection();
}

TSharedPtr<FTabManager::FLayout> FVoxelGraphToolkit::GetLayout() const
{
	return FTabManager::NewLayout("FVoxelGraphToolkit_Layout_v5")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.15f)
				->SetHideTabWell(true)
				->AddTab(MembersTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.7f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.8f)
					->SetHideTabWell(true)
					->AddTab("Document", ETabState::ClosedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.2f)
					->AddTab(MessagesTabId, ETabState::OpenedTab)
					->AddTab(SearchTabId, ETabState::OpenedTab)
					->SetForegroundTab(FTabId(MessagesTabId))
				)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Vertical)
				->SetSizeCoefficient(0.15f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.4f)
					->SetHideTabWell(true)
					->AddTab(ViewportTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.1f)
					->SetHideTabWell(true)
					->AddTab(PreviewStatsTabId, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.5f)
					->AddTab(DetailsTabId, ETabState::OpenedTab)
					->AddTab(PreviewDetailsTabId, ETabState::OpenedTab)
					->SetForegroundTab(FTabId(DetailsTabId))
				)
			)
		);
}

void FVoxelGraphToolkit::RegisterTabs(const FRegisterTab RegisterTab)
{
	RegisterTab(ViewportTabId, INVTEXT("Preview"), "LevelEditor.Tabs.Viewports", GraphPreview);
	RegisterTab(PreviewStatsTabId, INVTEXT("Stats"), "MaterialEditor.ToggleMaterialStats.Tab", GraphPreviewStats);
	RegisterTab(DetailsTabId, INVTEXT("Details"), "LevelEditor.Tabs.Details", DetailsView);
	RegisterTab(PreviewDetailsTabId, INVTEXT("Preview Settings"), "LevelEditor.Tabs.Details", PreviewDetailsView);
	RegisterTab(MembersTabId, INVTEXT("Members"), "ClassIcon.BlueprintCore", GraphMembers);
	RegisterTab(MessagesTabId, INVTEXT("Messages"), "MessageLog.TabIcon", GraphMessages);
	RegisterTab(SearchTabId, INVTEXT("Find Results"), "Kismet.Tabs.FindResults", SearchWidget);
}

void FVoxelGraphToolkit::SaveDocuments()
{
	Asset->LastEditedDocuments.Empty();
	DocumentManager->SaveAllState();
}

void FVoxelGraphToolkit::LoadDocuments()
{
	Asset->LastEditedDocuments.RemoveAll([](const FVoxelEditedDocumentInfo& Document)
	{
		return !Document.EdGraph.Get();
	});

	if (Asset->LastEditedDocuments.Num() == 0)
	{
		if (Asset->IsFunctionLibrary())
		{
			for (const FGuid& Guid : Asset->GetTerminalGraphs())
			{
				const UVoxelTerminalGraph* TerminalGraph = Asset->FindTerminalGraph(Guid);
				if (!ensure(TerminalGraph) ||
					TerminalGraph->IsMainTerminalGraph())
				{
					continue;
				}

				FVoxelEditedDocumentInfo DocumentInfo;
				DocumentInfo.EdGraph = &TerminalGraph->GetEdGraph();
				Asset->LastEditedDocuments.Add(DocumentInfo);
				break;
			}
		}
		else
		{
			FVoxelEditedDocumentInfo DocumentInfo;
			DocumentInfo.EdGraph = &Asset->GetMainTerminalGraph().GetEdGraph();
			Asset->LastEditedDocuments.Add(DocumentInfo);
		}
	}

	for (const FVoxelEditedDocumentInfo& Document : Asset->LastEditedDocuments)
	{
		const UEdGraph* EdGraph = Document.EdGraph.Get();
		if (!ensure(EdGraph))
		{
			continue;
		}

		const TSharedPtr<SDockTab> Tab = DocumentManager->OpenDocument(FTabPayload_UObject::Make(EdGraph), FDocumentTracker::RestorePreviousDocument);
		if (!ensure(Tab))
		{
			continue;
		}

		if (const TSharedPtr<SGraphEditor> GraphEditor = FVoxelGraphEditorSummoner::GetGraphEditor(Tab))
		{
			GraphEditor->SetViewLocation(Document.ViewLocation, Document.ZoomAmount);
		}
	}
}

void FVoxelGraphToolkit::PostUndo()
{
	Super::PostUndo();

	DocumentManager->RefreshAllTabs();
	DocumentManager->CleanInvalidTabs();

	// Clear selection, to avoid holding refs to nodes that go away

	Asset->ForeachTerminalGraph_NoInheritance([&](const UVoxelTerminalGraph& TerminalGraph)
	{
		if (const TSharedPtr<SGraphEditor> GraphEditor = FindGraphEditor(TerminalGraph.GetEdGraph()))
		{
			GraphEditor->ClearSelectionSet();
			GraphEditor->NotifyGraphChanged();
		}
	});
}

void FVoxelGraphToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(Collector);

	GraphPreview->AddReferencedObjects(Collector);
}

void FVoxelGraphToolkit::SetTabManager(const TSharedRef<FTabManager>& TabManager)
{
	Super::SetTabManager(TabManager);

	DocumentManager->SetTabManager(TabManager);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<SGraphEditor> FVoxelGraphToolkit::GetActiveGraphEditor() const
{
	return WeakFocusedGraph.Pin();
}

UEdGraph* FVoxelGraphToolkit::GetActiveEdGraph() const
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!GraphEditor)
	{
		return nullptr;
	}
	return GraphEditor->GetCurrentGraph();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphToolkit::FixupTerminalGraph(UVoxelTerminalGraph* TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(TerminalGraph))
	{
		return;
	}

	UVoxelEdGraph& EdGraph = TerminalGraph->GetTypedEdGraph();
	EdGraph.Schema = UVoxelGraphSchema::StaticClass();
	EdGraph.SetToolkit(SharedThis(this));
	EdGraph.MigrateAndReconstructAll();

	TerminalGraph->GetRuntime().OnMessagesChanged.RemoveAll(this);
	TerminalGraph->GetRuntime().OnMessagesChanged.Add(MakeWeakPtrDelegate(this, [=]
	{
		bMessageUpdateQueued = true;
	}));
}

TSharedPtr<SGraphEditor> FVoxelGraphToolkit::FindGraphEditor(const UEdGraph& EdGraph) const
{
	for (const TSharedPtr<SDockTab>& Tab : DocumentManager->GetAllDocumentTabs())
	{
		if (!ensure(Tab))
		{
			continue;
		}

		if (const TSharedPtr<SGraphEditor> GraphEditor = FVoxelGraphEditorSummoner::GetGraphEditor(Tab))
		{
			if (GraphEditor->GetCurrentGraph() == &EdGraph)
			{
				return GraphEditor;
			}
		}
	}

	return nullptr;
}

TSharedRef<SGraphEditor> FVoxelGraphToolkit::CreateGraphEditor(UEdGraph& EdGraph, const bool bBindOnSelect)
{
	VOXEL_FUNCTION_COUNTER();

	// Make sure to fixup the graph before opening it
	FixupTerminalGraph(EdGraph.GetTypedOuter<UVoxelTerminalGraph>());

	const FGraphAppearanceInfo AppearanceInfo;

	SGraphEditor::FGraphEditorEvents Events;
	if (bBindOnSelect)
	{
		Events.OnSelectionChanged = MakeLambdaDelegate([
			=,
			WeakTerminalGraph = MakeWeakObjectPtr(EdGraph.GetTypedOuter<UVoxelTerminalGraph>())
		](const TSet<UObject*>& NewSelection)
		{
			const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
			if (!ensure(TerminalGraph))
			{
				return;
			}

			if (NewSelection.Num() == 0)
			{
				Selection->SelectTerminalGraph(*TerminalGraph);
			}
			else if (NewSelection.Num() == 1)
			{
				Selection->SelectNode(CastChecked<UEdGraphNode>(NewSelection.Array()[0]));
			}
			else
			{
				Selection->SelectNone();
			}

			if (NewSelection.Num() == 1)
			{
				PreviewDetailsView->SetObject(NewSelection.Array()[0]);
			}
			else
			{
				PreviewDetailsView->SetObject(nullptr);
			}
		});
	}
	Events.OnTextCommitted = MakeLambdaDelegate([](const FText& NewText, ETextCommit::Type, UEdGraphNode* NodeBeingChanged)
	{
		if (!ensure(NodeBeingChanged))
		{
			return;
		}

		const FVoxelTransaction Transaction(NodeBeingChanged, "Rename Node");
		NodeBeingChanged->OnRenameNode(NewText.ToString());
	});
	Events.OnNodeDoubleClicked = MakeLambdaDelegate([](UEdGraphNode* Node)
	{
		if (Node->CanJumpToDefinition())
		{
			Node->JumpToDefinition();
		}
	});
	Events.OnSpawnNodeByShortcut = MakeWeakPtrDelegate(this, [this](FInputChord Chord, const FVector2D& Position)
	{
		// TODO?
		return FReply::Handled();
	});

	Events.OnCreateActionMenu = MakeLambdaDelegate([](
		UEdGraph* InGraph,
		const FVector2D& InNodePosition,
		const TArray<UEdGraphPin*>& InDraggedPins,
		const bool bAutoExpand,
		SGraphEditor::FActionMenuClosed InOnMenuClosed)
	{
		const TSharedRef<SVoxelGraphEditorActionMenu> Menu =
			SNew(SVoxelGraphEditorActionMenu)
			.GraphObj(InGraph)
			.NewNodePosition(InNodePosition)
			.DraggedFromPins(InDraggedPins)
			.AutoExpandActionMenu(bAutoExpand)
			.OnClosedCallback(InOnMenuClosed);

		return FActionMenuContent(Menu, Menu->GetFilterTextBox());
	});

	return
		SNew(SGraphEditor)
		.AdditionalCommands(GetCommands())
		.Appearance(AppearanceInfo)
		.GraphToEdit(&EdGraph)
		.GraphEvents(Events)
		.AutoExpandActionMenu(false)
		.ShowGraphStateOverlay(false);
}

TSharedPtr<SGraphEditor> FVoxelGraphToolkit::OpenGraphAndBringToFront(const UEdGraph& EdGraph, const bool bSetFocus) const
{
	const TSharedPtr<SDockTab> Tab = DocumentManager->OpenDocument(FTabPayload_UObject::Make(&EdGraph), FDocumentTracker::OpenNewDocument);
	if (!ensure(Tab))
	{
		return nullptr;
	}

	const TSharedPtr<SGraphEditor> GraphEditor = FVoxelGraphEditorSummoner::GetGraphEditor(Tab);
	if (!ensure(GraphEditor))
	{
		return nullptr;
	}

	if (bSetFocus)
	{
		GraphEditor->CaptureKeyboard();
	}
	return GraphEditor;
}

void FVoxelGraphToolkit::CloseGraph(UEdGraph& EdGraph)
{
	ensure(!GraphBeingClosed.IsValid());
	GraphBeingClosed = &EdGraph;
	{
		DocumentManager->CloseTab(FTabPayload_UObject::Make(&EdGraph));
	}
	ensure(GraphBeingClosed == &EdGraph);
	GraphBeingClosed.Reset();
}

TSet<UEdGraphNode*> FVoxelGraphToolkit::GetSelectedNodes() const
{
	const TSharedPtr<SGraphEditor> GraphEditor = GetActiveGraphEditor();
	if (!GraphEditor)
	{
		return {};
	}

	// Removed nodes might still be selected & might only be unselected on the next frame as
	// removal might still be pending from GraphEditor
	const TSet<UEdGraphNode*> ValidNodes(GraphEditor->GetCurrentGraph()->Nodes);

	TSet<UEdGraphNode*> SelectedNodes;
	for (UObject* Object : GraphEditor->GetSelectedNodes())
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
		if (!ensure(Node) ||
			!ValidNodes.Contains(Node))
		{
			continue;
		}

		SelectedNodes.Add(Node);
	}
	return SelectedNodes;
}

void FVoxelGraphToolkit::AddNodeToReconstruct(UEdGraphNode* Node)
{
	NodesToReconstruct.Add(Node);
}