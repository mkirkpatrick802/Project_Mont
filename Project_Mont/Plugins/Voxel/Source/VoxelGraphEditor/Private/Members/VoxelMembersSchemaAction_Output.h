// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Members/VoxelMembersDragDropAction_Base.h"
#include "VoxelMembersSchemaAction_VariableBase.h"

class UVoxelTerminalGraph;
struct FVoxelGraphToolkit;

struct FVoxelMembersSchemaAction_Output : public FVoxelMembersSchemaAction_VariableBase
{
public:
	TWeakObjectPtr<UVoxelTerminalGraph> WeakTerminalGraph;

	using FVoxelMembersSchemaAction_VariableBase::FVoxelMembersSchemaAction_VariableBase;

	static void OnPaste(
		const FVoxelGraphToolkit& Toolkit,
		UVoxelTerminalGraph& TerminalGraph,
		const FString& Text,
		const FString& Category);

	//~ Begin FVoxelMembersSchemaAction_Base Interface
	virtual UObject* GetOuter() const override;
	virtual void ApplyNewGuids(const TArray<FGuid>& NewGuids) const override;
	virtual FReply OnDragged(const FPointerEvent& MouseEvent) override;
	virtual void BuildContextMenu(FMenuBuilder& MenuBuilder) override;
	virtual void OnActionDoubleClick() const override;
	virtual void OnSelected() const override;
	virtual void OnDelete() const override;
	virtual void OnDuplicate() const override;
	virtual bool OnCopy(FString& OutText) const override;
	virtual FString GetName() const override;
	virtual void SetName(const FString& Name) const override;
	virtual void SetCategory(const FString& NewCategory) const override;
	virtual FString GetSearchString() const override;
	//~ End FVoxelMembersSchemaAction_Base Interface

	//~ Begin FVoxelMembersSchemaAction_VariableBase Interface
	virtual FVoxelPinType GetPinType() const override;
	virtual void SetPinType(const FVoxelPinType& NewPinType) const override;
	//~ End FVoxelMembersSchemaAction_VariableBase Interface
};

class FVoxelMembersDragDropAction_Output : public FVoxelMembersDragDropAction_Base
{
public:
	DRAG_DROP_OPERATOR_TYPE(FVoxelMembersDragDropAction_Output, FVoxelMembersDragDropAction_Base);

	const TWeakObjectPtr<UVoxelTerminalGraph> WeakTerminalGraph;
	const FGuid OutputGuid;

	static TSharedRef<FVoxelMembersDragDropAction_Output> New(
		const TSharedRef<FVoxelMembersSchemaAction_Base>& Action,
		const TWeakObjectPtr<UVoxelTerminalGraph>& WeakTerminalGraph,
		const FGuid& OutputGuid)
	{
		const TSharedRef<FVoxelMembersDragDropAction_Output> Operation = MakeVoxelShareable(new (GVoxelMemory) FVoxelMembersDragDropAction_Output(
			WeakTerminalGraph,
			OutputGuid));
		Operation->Construct(Action);
		return Operation;
	}

	//~ Begin FGraphSchemaActionDragDropAction Interface
	virtual void HoverTargetChanged() override;

	virtual FReply DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnNode(FVector2D ScreenPosition, FVector2D GraphPosition) override;
	virtual FReply DroppedOnPanel(
		const TSharedRef<SWidget>& Panel,
		FVector2D ScreenPosition,
		FVector2D GraphPosition,
		UEdGraph& EdGraph) override;

	virtual void GetDefaultStatusSymbol(
		const FSlateBrush*& PrimaryBrushOut,
		FSlateColor& IconColorOut,
		FSlateBrush const*& SecondaryBrushOut,
		FSlateColor& SecondaryColorOut) const override;
	//~ End FGraphSchemaActionDragDropAction Interface

private:
	FVoxelMembersDragDropAction_Output(
		const TWeakObjectPtr<UVoxelTerminalGraph>& WeakTerminalGraph,
		const FGuid& OutputGuid)
		: WeakTerminalGraph(WeakTerminalGraph)
		, OutputGuid(OutputGuid)
	{
	}
};