// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelColumnSizeData.h"
#include "Material/VoxelMaterialDefinition.h"

class SGraphActionMenu;
struct FVoxelMaterialDefinitionToolkit;
struct FVoxelMaterialDefinitionParameterSchemaAction;

class SVoxelMaterialDefinitionParameters
	: public SCompoundWidget
	, public FSelfRegisteringEditorUndoClient
{
public:
	const TSharedRef<FVoxelColumnSizeData> ColumnSizeData = MakeVoxelShared<FVoxelColumnSizeData>();

	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<FVoxelMaterialDefinitionToolkit>, Toolkit);
	};

	void Construct(const FArguments& Args);

public:
	void SelectMember(const FGuid& Guid, int32 SectionId, bool bRequestRename, bool bRefresh);

	TSharedPtr<FVoxelMaterialDefinitionToolkit> GetToolkit() const
	{
		return WeakToolkit.Pin();
	}
	void QueueRefresh()
	{
		bRefreshQueued = true;
	}

public:
	//~ Begin SWidget Interface
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End SWidget Interface

public:
	//~ Begin FSelfRegisteringEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FSelfRegisteringEditorUndoClient Interface

private:
	void Refresh();
	FString GetPasteCategory() const;
	void OnAddNewMember() const;
	TSharedPtr<SWidget> OnContextMenuOpening();
	TSharedPtr<FVoxelMaterialDefinitionParameterSchemaAction> GetSelectedAction() const;

private:
	const TSharedRef<FUICommandList> CommandList = MakeVoxelShared<FUICommandList>();

	bool bIsRefreshing = false;
	bool bRefreshQueued = false;

	TWeakPtr<FVoxelMaterialDefinitionToolkit> WeakToolkit;
	TSharedPtr<SSearchBox> FilterBox;
	TSharedPtr<SGraphActionMenu> MembersMenu;
};