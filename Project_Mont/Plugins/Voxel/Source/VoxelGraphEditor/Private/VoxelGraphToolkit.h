// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "VoxelToolkit.h"
#include "VoxelGraphCommandManager.h"
#include "Selection/VoxelGraphSelection.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "VoxelGraphToolkit.generated.h"

class FDocumentTracker;
class SVoxelGraphSearch;
class SVoxelGraphMembers;
class SVoxelGraphPreview;
class SVoxelGraphMessages;

USTRUCT(meta = (Internal))
struct FVoxelGraphToolkit : public FVoxelToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelGraph> Asset;

public:
	TSharedRef<SVoxelGraphMembers> GetGraphMembers() const
	{
		return GraphMembers.ToSharedRef();
	}
	TSharedRef<SVoxelGraphSearch> GetSearchWidget() const
	{
		return SearchWidget.ToSharedRef();
	}
	FVoxelGraphSelection& GetSelection() const
	{
		return *Selection;
	}
	FVoxelGraphCommandManager& GetCommandManager() const
	{
		return *CommandManager;
	}
	IDetailsView& GetPreviewDetailsView() const
	{
		return *PreviewDetailsView;
	}

public:
	static TSharedPtr<FVoxelGraphToolkit> Get(const UEdGraph* EdGraph);

public:
	//~ Begin FVoxelToolkit Interface
	virtual void Initialize() override;
	virtual void Tick() override;
	virtual void BuildMenu(FMenuBarBuilder& MenuBarBuilder) override;
	virtual void BuildToolbar(FToolBarBuilder& ToolbarBuilder) override;
	virtual TSharedPtr<FTabManager::FLayout> GetLayout() const override;
	virtual void RegisterTabs(FRegisterTab RegisterTab) override;
	virtual void SaveDocuments() override;
	virtual void LoadDocuments() override;
	virtual void PostUndo() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual void SetTabManager(const TSharedRef<FTabManager>& TabManager) override;
	//~ End FVoxelToolkit Interface

public:
	TSharedPtr<SGraphEditor> GetActiveGraphEditor() const;
	UEdGraph* GetActiveEdGraph() const;

public:
	void FixupTerminalGraph(UVoxelTerminalGraph* TerminalGraph);
	TSharedPtr<SGraphEditor> FindGraphEditor(const UEdGraph& EdGraph) const;
	TSharedRef<SGraphEditor> CreateGraphEditor(UEdGraph& EdGraph, bool bBindOnSelect);
	TSharedPtr<SGraphEditor> OpenGraphAndBringToFront(const UEdGraph& EdGraph, bool bSetFocus) const;
	void CloseGraph(UEdGraph& EdGraph);
	TSet<UEdGraphNode*> GetSelectedNodes() const;
	void AddNodeToReconstruct(UEdGraphNode* Node);

public:
	static constexpr const TCHAR* ViewportTabId = TEXT("FVoxelGraphEditorToolkit_Viewport");
	static constexpr const TCHAR* DetailsTabId = TEXT("FVoxelGraphEditorToolkit_Details");
	static constexpr const TCHAR* PreviewDetailsTabId = TEXT("FVoxelGraphEditorToolkit_PreviewDetails");
	static constexpr const TCHAR* MessagesTabId = TEXT("FVoxelGraphEditorToolkit_Messages");
	static constexpr const TCHAR* GraphTabId = TEXT("FVoxelGraphEditorToolkit_Graph");
	static constexpr const TCHAR* MembersTabId = TEXT("FVoxelGraphEditorToolkit_Members");
	static constexpr const TCHAR* PreviewStatsTabId = TEXT("FVoxelGraphEditorToolkit_PreviewStats");
	static constexpr const TCHAR* SearchTabId = TEXT("FVoxelGraphEditorToolkit_Search");

private:
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<IDetailsView> PreviewDetailsView;
	TSharedPtr<SVoxelGraphMessages> GraphMessages;
	TSharedPtr<SWidget> GraphPreviewStats;
	TSharedPtr<SVoxelGraphPreview> GraphPreview;
	TSharedPtr<SVoxelGraphMembers> GraphMembers;
	TSharedPtr<SVoxelGraphSearch> SearchWidget;

	TSharedPtr<FDocumentTracker> DocumentManager;
	TSharedPtr<FVoxelGraphSelection> Selection;
	TSharedPtr<FVoxelGraphCommandManager> CommandManager;

	bool bMessageUpdateQueued = false;

	TSet<TWeakObjectPtr<UEdGraphNode>> NodesToReconstruct;

	TWeakPtr<SGraphEditor> WeakFocusedGraph;
	TWeakObjectPtr<UEdGraph> GraphBeingClosed;

	friend struct FVoxelGraphEditorSummoner;
};