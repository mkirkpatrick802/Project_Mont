// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphEditorSummoner.h"
#include "VoxelGraphToolkit.h"
#include "VoxelPluginVersion.h"
#include "VoxelTerminalGraph.h"
#include "Widgets/SVoxelGraphPreview.h"
#include "Members/SVoxelGraphMembers.h"

FVoxelGraphEditorSummoner::FVoxelGraphEditorSummoner(const TSharedRef<FVoxelGraphToolkit>& Toolkit)
	: FDocumentTabFactoryForObjects<UEdGraph>(STATIC_FNAME("FVoxelGraphEditorSummoner"), nullptr)
	, WeakToolkit(Toolkit)
{
}

void FVoxelGraphEditorSummoner::OnTabActivated(const TSharedPtr<SDockTab> Tab) const
{
	Tab->SetOnTabClosed(MakeWeakPtrDelegate(this, [this](const TSharedRef<SDockTab> InTab)
	{
		const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();
		if (!ensure(Toolkit))
		{
			return;
		}

		if (Toolkit->WeakFocusedGraph == GetGraphEditor(InTab))
		{
			FocusGraphEditor(nullptr);
		}
	}));

	FocusGraphEditor(GetGraphEditor(Tab));
}

void FVoxelGraphEditorSummoner::SaveState(
	const TSharedPtr<SDockTab> Tab,
	const TSharedPtr<FTabPayload> Payload) const
{
	if (!ensure(Payload->IsValid()))
	{
		return;
	}

	const UEdGraph* Graph = FTabPayload_UObject::CastChecked<UEdGraph>(Payload);
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();

	if (!ensure(Graph) ||
		!ensure(Toolkit))
	{
		return;
	}

	FVector2D ViewLocation = FVector2D::ZeroVector;
	float ZoomAmount = 0.f;
	if (const TSharedPtr<SGraphEditor> GraphEditor = GetGraphEditor(Tab))
	{
		GraphEditor->GetViewLocation(ViewLocation, ZoomAmount);
	}

	FVoxelEditedDocumentInfo DocumentInfo;
	DocumentInfo.EdGraph = Graph;
	DocumentInfo.ViewLocation = ViewLocation;
	DocumentInfo.ZoomAmount	 = ZoomAmount;
	Toolkit->Asset->LastEditedDocuments.Add(DocumentInfo);
}

TAttribute<FText> FVoxelGraphEditorSummoner::ConstructTabNameForObject(UEdGraph* EdGraph) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(Toolkit))
	{
		return {};
	}

	return TAttribute<FText>::CreateLambda([=]() -> FText
	{
		if (!ensure(EdGraph))
		{
			return {};
		}

		const UVoxelTerminalGraph* TerminalGraph = EdGraph->GetTypedOuter<UVoxelTerminalGraph>();
		if (!ensure(TerminalGraph))
		{
			return {};
		}

		if (TerminalGraph->IsMainTerminalGraph())
		{
			return INVTEXT("Main Graph");
		}

		return FText::FromString(TerminalGraph->GetDisplayName());
	});
}

TSharedRef<SWidget> FVoxelGraphEditorSummoner::CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* EdGraph) const
{
	if (!ensure(EdGraph))
	{
		return SNullWidget::NullWidget;
	}

	const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();
	check(Toolkit);

	return
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			Toolkit->CreateGraphEditor(*EdGraph, true)
		]
		+ SOverlay::Slot()
		.Padding(10)
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Right)
		[
			SNew(SVerticalBox)
			.Visibility(EVisibility::HitTestInvisible)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, -10.f)
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "Graph.CornerText")
				.Text(INVTEXT("VOXEL"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "Graph.CornerText")
				.Font(FCoreStyle::GetDefaultFontStyle("BoldCondensed", 16))
				.Text(FText::FromString(FVoxelUtilities::GetPluginVersion().ToString_UserFacing()))
			]
		];
}

const FSlateBrush* FVoxelGraphEditorSummoner::GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* EdGraph) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(EdGraph) ||
		!ensure(Toolkit))
	{
		return nullptr;
	}

	const UVoxelTerminalGraph* TerminalGraph = EdGraph->GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(TerminalGraph))
	{
		return nullptr;
	}

	if (TerminalGraph->HasExecNodes())
	{
		return FVoxelEditorStyle::GetBrush("VoxelGraph.Execute");
	}
	else
	{
		return FAppStyle::GetBrush("GraphEditor.Function_16x");
	}
}

TSharedRef<FGenericTabHistory> FVoxelGraphEditorSummoner::CreateTabHistoryNode(TSharedPtr<FTabPayload> Payload)
{
	return MakeVoxelShared<FVoxelGraphTabHistory>(SharedThis(this), Payload);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<SGraphEditor> FVoxelGraphEditorSummoner::GetGraphEditor(const TSharedPtr<SDockTab>& Tab)
{
	if (Tab->GetContent()->GetType() != STATIC_FNAME("SOverlay"))
	{
		return nullptr;
	}

	SOverlay& Overlay = static_cast<SOverlay&>(*Tab->GetContent());
	const TSharedRef<SWidget> OverlayChild = Overlay.GetChildren()->GetChildAt(0);
	if (!ensure(OverlayChild->GetType() == STATIC_FNAME("SGraphEditor")))
	{
		return nullptr;
	}

	return StaticCastSharedPtr<SGraphEditor>(OverlayChild.ToSharedPtr());
}

void FVoxelGraphEditorSummoner::FocusGraphEditor(const TSharedPtr<SGraphEditor>& GraphEditor) const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = WeakToolkit.Pin();
	if (!ensure(Toolkit))
	{
		return;
	}

	if (Toolkit->WeakFocusedGraph == GraphEditor)
	{
		return;
	}

	Toolkit->WeakFocusedGraph = GraphEditor;

	if (!GraphEditor)
	{
		Toolkit->GraphPreview->SetTerminalGraph(nullptr);
		Toolkit->GraphMembers->SetTerminalGraph(nullptr);
		return;
	}

	if (Toolkit->GraphBeingClosed == GraphEditor->GetCurrentGraph())
	{
		return;
	}

	const UEdGraph* EdGraph = GraphEditor->GetCurrentGraph();
	if (!ensure(EdGraph))
	{
		return;
	}

	UVoxelTerminalGraph* TerminalGraph = EdGraph->GetTypedOuter<UVoxelTerminalGraph>();
	if (!TerminalGraph)
	{
		return;
	}

	Toolkit->GraphPreview->SetTerminalGraph(TerminalGraph);
	Toolkit->GraphMembers->SetTerminalGraph(TerminalGraph);
	Toolkit->GetSelection().Update();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphTabHistory::EvokeHistory(const TSharedPtr<FTabInfo> InTabInfo, const bool bPrevTabMatches)
{
	const TSharedPtr<SDockTab> DockTab = InTabInfo->GetTab().Pin();
	if (!ensure(DockTab))
	{
		return;
	}

	if (bPrevTabMatches)
	{
		WeakGraphEditor = FVoxelGraphEditorSummoner::GetGraphEditor(DockTab);
		return;
	}

	const TSharedPtr<FDocumentTabFactory> Factory = FactoryPtr.Pin();
	if (!ensure(Factory))
	{
		return;
	}

	FWorkflowTabSpawnInfo SpawnInfo;
	SpawnInfo.Payload = Payload;
	SpawnInfo.TabInfo = InTabInfo;

	const TSharedRef<SOverlay> Overlay = StaticCastSharedRef<SOverlay>(Factory->CreateTabBody(SpawnInfo));
	const TSharedRef<SGraphEditor> GraphEditor = StaticCastSharedRef<SGraphEditor>(Overlay->GetChildren()->GetChildAt(0));
	WeakGraphEditor = GraphEditor;
	Factory->UpdateTab(DockTab, SpawnInfo, Overlay);
}

void FVoxelGraphTabHistory::SaveHistory()
{
	if (!IsHistoryValid())
	{
		return;
	}

	const TSharedPtr<SGraphEditor> GraphEditor = WeakGraphEditor.Pin();
	if (!ensure(GraphEditor))
	{
		return;
	}

	GraphEditor->GetViewLocation(SavedLocation, SavedZoomAmount);
	GraphEditor->GetViewBookmark(SavedBookmarkId);
}

void FVoxelGraphTabHistory::RestoreHistory()
{
	if (!IsHistoryValid())
	{
		return;
	}

	const TSharedPtr<SGraphEditor> GraphEditor = WeakGraphEditor.Pin();
	if (!ensure(GraphEditor))
	{
		return;
	}

	GraphEditor->SetViewLocation(SavedLocation, SavedZoomAmount, SavedBookmarkId);
}