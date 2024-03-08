// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SGraphActionMenu.h"
#include "VoxelColumnSizeData.h"
#include "VoxelMembersSchemaAction_Base.h"

class UVoxelTerminalGraph;
class UVoxelGraph;
struct FVoxelGraphToolkit;

enum class EVoxelGraphMemberSection
{
	MainGraph,
	Functions,
	Parameters,
	GraphInputs,
	FunctionInputs,
	Outputs,
	LocalVariables,
};
ENUM_RANGE_BY_FIRST_AND_LAST(EVoxelGraphMemberSection, EVoxelGraphMemberSection::MainGraph, EVoxelGraphMemberSection::LocalVariables);

class SVoxelGraphMembers
	: public SCompoundWidget
	, public FSelfRegisteringEditorUndoClient
{
public:
	FORCEINLINE static int32 GetSectionId(const EVoxelGraphMemberSection Type)
	{
		switch (Type)
		{
		default: check(false);
		// Can't use 0, it's used internally by SGraphActionMenu
		case EVoxelGraphMemberSection::MainGraph: return 1;
		case EVoxelGraphMemberSection::Functions: return 2;
		case EVoxelGraphMemberSection::Parameters: return 3;
		case EVoxelGraphMemberSection::GraphInputs: return 4;
		case EVoxelGraphMemberSection::FunctionInputs: return 5;
		case EVoxelGraphMemberSection::Outputs: return 6;
		case EVoxelGraphMemberSection::LocalVariables: return 7;
		}
	}
	FORCEINLINE static FString LexToString(const EVoxelGraphMemberSection Type)
	{
		switch (Type)
		{
		default: check(false);
		case EVoxelGraphMemberSection::MainGraph: return "MainGraph";
		case EVoxelGraphMemberSection::Functions: return "Functions";
		case EVoxelGraphMemberSection::Parameters: return "Parameters";
		case EVoxelGraphMemberSection::GraphInputs: return "Graph Inputs";
		case EVoxelGraphMemberSection::FunctionInputs: return "Function Inputs";
		case EVoxelGraphMemberSection::Outputs: return "Outputs";
		case EVoxelGraphMemberSection::LocalVariables: return "LocalVariables";
		}
	}
	FORCEINLINE static EVoxelGraphMemberSection GetSection(const int32 SectionId)
	{
		switch (SectionId)
		{
		default: ensure(false);
		case 1: return EVoxelGraphMemberSection::MainGraph;
		case 2: return EVoxelGraphMemberSection::Functions;
		case 3: return EVoxelGraphMemberSection::Parameters;
		case 4: return EVoxelGraphMemberSection::GraphInputs;
		case 5: return EVoxelGraphMemberSection::FunctionInputs;
		case 6: return EVoxelGraphMemberSection::Outputs;
		case 7: return EVoxelGraphMemberSection::LocalVariables;
		}
	}

public:
	const TSharedRef<FVoxelColumnSizeData> ColumnSizeData = MakeVoxelShared<FVoxelColumnSizeData>();

	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<FVoxelGraphToolkit>, Toolkit);
	};
	void Construct(const FArguments& Args);

	//~ Begin SWidget Interface
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	//~ End SWidget Interface

public:
	void Refresh();
	void RequestRename();
	void SetTerminalGraph(const TWeakObjectPtr<UVoxelTerminalGraph>& NewTerminalGraph);

public:
	void SelectNone() const;

	void SelectCategory(
		int32 SectionId,
		FName DisplayName) const;

	void SelectMember(
		EVoxelGraphMemberSection Section,
		const FGuid& Guid) const;

	void SelectClosestAction(
		int32 SectionId,
		const FGuid& Guid) const;

public:
	TArray<TSharedRef<FVoxelMembersSchemaAction_Base>> GetActions();

protected:
	//~ Begin FSelfRegisteringEditorUndoClient Interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
	//~ End FSelfRegisteringEditorUndoClient Interface

public:
	TSharedPtr<FVoxelGraphToolkit> GetToolkit() const
	{
		return WeakToolkit.Pin();
	}
	UVoxelTerminalGraph* GetTerminalGraph() const
	{
		return WeakTerminalGraph.Get();
	}

private:
	FString GetPasteCategory() const;
	void OnAddNewMember(int32 SectionId);
	TSharedPtr<FVoxelMembersSchemaAction_Base> GetSelectedAction() const;

private:
	static bool TryParsePastedText(FString& InOutText, int32& OutSectionId);
	static void SetupTerminalGraphOverride(UVoxelTerminalGraph& TerminalGraph);

	bool bIsRefreshing = false;

	TSharedPtr<SSearchBox> FilterBox;
	TSharedPtr<FUICommandList> CommandList;

private:
	class SVoxelGraphActionMenu : public SGraphActionMenu
	{
	public:
		TArray<TSharedPtr<FGraphActionNode>> GetSelectedNodes() const
		{
			return TreeView->GetSelectedItems();
		}
		const TArray<TSharedPtr<FGraphActionNode>>& GetVisibleNodes() const
		{
			return FilteredActionNodes;
		}
	};

	TSharedPtr<SVoxelGraphActionMenu> MembersMenu;

private:
	FSharedVoidPtr OnPropertyChangedPtr;
	TWeakPtr<FVoxelGraphToolkit> WeakToolkit;
	TWeakObjectPtr<UVoxelTerminalGraph> WeakTerminalGraph;
};