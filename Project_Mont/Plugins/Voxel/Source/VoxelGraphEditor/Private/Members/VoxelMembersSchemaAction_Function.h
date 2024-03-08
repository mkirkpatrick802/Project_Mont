// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Members/VoxelMembersSchemaAction_Base.h"
#include "Members/VoxelMembersDragDropAction_Base.h"

class UVoxelGraph;
class UVoxelTerminalGraph;
struct FVoxelGraphToolkit;

struct FVoxelMembersSchemaAction_Function : public FVoxelMembersSchemaAction_Base
{
public:
	TWeakObjectPtr<UVoxelTerminalGraph> WeakTerminalGraph;

	using FVoxelMembersSchemaAction_Base::FVoxelMembersSchemaAction_Base;

	static void OnPaste(
		FVoxelGraphToolkit& Toolkit,
		const UVoxelTerminalGraph* TerminalGraphToCopy);

	//~ Begin FVoxelMembersSchemaAction_Base Interface
	virtual UObject* GetOuter() const override;
	virtual TSharedRef<SWidget> CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const override;
	virtual void ApplyNewGuids(const TArray<FGuid>& NewGuids) const override;
	virtual FReply OnDragged(const FPointerEvent& MouseEvent) override;
	virtual void BuildContextMenu(FMenuBuilder& MenuBuilder) override;
	virtual void OnActionDoubleClick() const override;
	virtual void OnSelected() const override;
	virtual void OnDelete() const override;
	virtual void OnDuplicate() const override;
	virtual bool OnCopy(FString& OutText) const override;
	virtual bool CanBeRenamed() const override;
	virtual bool CanBeDeleted() const override { return true; }
	virtual FString GetName() const override;
	virtual void SetName(const FString& Name) const override;
	virtual void SetCategory(const FString& NewCategory) const override;
	//~ End FVoxelMembersSchemaAction_Base Interface
};

class SVoxelMembersPaletteItem_Function : public SVoxelMembersPaletteItem<FVoxelMembersSchemaAction_Function>
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& InArgs, const FCreateWidgetForActionData& CreateData);
};

class FVoxelMembersDragDropAction_Function : public FVoxelMembersDragDropAction_Base
{
public:
	DRAG_DROP_OPERATOR_TYPE(FVoxelGraphMacroDragDropAction, FVoxelMembersDragDropAction_Base);

	const TWeakObjectPtr<UVoxelTerminalGraph> WeakTerminalGraph;

	static TSharedRef<FVoxelMembersDragDropAction_Function> New(
		const TSharedRef<FVoxelMembersSchemaAction_Base>& Action,
		const TWeakObjectPtr<UVoxelTerminalGraph>& WeakTerminalGraph)
	{
		const TSharedRef<FVoxelMembersDragDropAction_Function> Operation = MakeVoxelShareable(new (GVoxelMemory) FVoxelMembersDragDropAction_Function(WeakTerminalGraph));
		Operation->Construct(Action);
		return Operation;
	}

	//~ Begin FGraphSchemaActionDragDropAction Interface
	virtual void HoverTargetChanged() override;
	virtual FReply DroppedOnPanel(
		const TSharedRef<SWidget>& Panel,
		FVector2D ScreenPosition,
		FVector2D GraphPosition,
		UEdGraph& EdGraph) override;
	//~ End FGraphSchemaActionDragDropAction Interface

private:
	explicit FVoxelMembersDragDropAction_Function(const TWeakObjectPtr<UVoxelTerminalGraph>& WeakTerminalGraph)
		: WeakTerminalGraph(WeakTerminalGraph)
	{
	}
};