// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"

struct FVoxelGraphToolkit;

struct FVoxelGraphEditorSummoner : public FDocumentTabFactoryForObjects<UEdGraph>
{
public:
	const TWeakPtr<FVoxelGraphToolkit> WeakToolkit;

	explicit FVoxelGraphEditorSummoner(const TSharedRef<FVoxelGraphToolkit>& Toolkit);

	//~ Begin FDocumentTabFactoryForObjects Interface
	virtual void OnTabActivated(TSharedPtr<SDockTab> Tab) const override;
	virtual void SaveState(TSharedPtr<SDockTab> Tab, TSharedPtr<FTabPayload> Payload) const override;

	virtual TAttribute<FText> ConstructTabNameForObject(UEdGraph* EdGraph) const override;
	virtual TSharedRef<SWidget> CreateTabBodyForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* EdGraph) const override;
	virtual const FSlateBrush* GetTabIconForObject(const FWorkflowTabSpawnInfo& Info, UEdGraph* EdGraph) const override;
	virtual TSharedRef<FGenericTabHistory> CreateTabHistoryNode(TSharedPtr<FTabPayload> Payload) override;
	//~ End FDocumentTabFactoryForObjects Interface

	static TSharedPtr<SGraphEditor> GetGraphEditor(const TSharedPtr<SDockTab>& Tab);

private:
	void FocusGraphEditor(const TSharedPtr<SGraphEditor>& GraphEditor) const;
};

struct FVoxelGraphTabHistory : public FGenericTabHistory
{
public:
	using FGenericTabHistory::FGenericTabHistory;

	//~ Begin FGenericTabHistory Interface
	virtual void EvokeHistory(TSharedPtr<FTabInfo> InTabInfo, bool bPrevTabMatches) override;
	virtual void SaveHistory() override;
	virtual void RestoreHistory() override;
	//~ End FGenericTabHistory Interface

private:
	TWeakPtr<SGraphEditor> WeakGraphEditor;
	FVector2D SavedLocation = FVector2D::ZeroVector;
	float SavedZoomAmount = -1.f;
	FGuid SavedBookmarkId;
};