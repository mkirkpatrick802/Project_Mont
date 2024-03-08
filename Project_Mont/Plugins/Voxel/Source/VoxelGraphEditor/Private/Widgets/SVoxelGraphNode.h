// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelGraphNodeBase.h"
#include "VoxelNodeDefinition.h"
#include "Nodes/VoxelGraphNode.h"

class SVoxelGraphNode : public SVoxelGraphNodeBase
{
private:
	struct FOverlayWidget
	{
		TSharedPtr<SWidget> Widget;
		FVector2D BrushSize;

		FVector2D GetLocation(const FVector2D& WidgetSize, const float Offset) const
		{
			return FVector2D(WidgetSize.X - BrushSize.X * 0.5f - Offset, BrushSize.Y * -0.5f);
		}
	};

public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& Args, UVoxelGraphNode* InNode);

	UVoxelGraphNode& GetVoxelNode() const
	{
		return *CastChecked<UVoxelGraphNode>(GraphNode);
	}

protected:
	//~ Begin SGraphNode Interface
	virtual void UpdateGraphNode() override;
	virtual void CreateOutputSideAddButton(TSharedPtr<SVerticalBox> OutputBox) override;
	virtual EVisibility IsAddPinButtonVisible() const override;
	virtual FReply OnAddPin() override;
	virtual void RequestRenameOnSpawn() override;
	virtual TArray<FOverlayWidgetInfo> GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const override;
	virtual void GetOverlayBrushes(bool bSelected, FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;
	//~ End SGraphNode Interface

	virtual void UpdateBottomContent(const TSharedPtr<SVerticalBox>& MainBox) {}

private:
	void UpdateStandardNode();
	void UpdateCompactNode();

	void CreateCategorizedPinWidgets();

	void CreateCategoryPinWidgets(
		const TSharedRef<IVoxelNodeDefinition::FNode>& Node,
		TArray<UEdGraphPin*>& Pins,
		TMap<FName, TArray<UEdGraphPin*>>& ParentToSplitPins,
		const TSharedPtr<SVerticalBox>& TargetContainer,
		bool bInput);

	void AddStandardNodePin(
		UEdGraphPin* Pin,
		const TSharedPtr<IVoxelNodeDefinition::FNode>& Node,
		const TSharedPtr<SVerticalBox>& TargetContainer);

	TSharedRef<SVerticalBox> CreateCategoryWidget(
		const TSharedRef<IVoxelNodeDefinition::FNode>& Node,
		bool bIsInput);
	void CreateCategoryVariadicButtons(
		const TSharedRef<IVoxelNodeDefinition::FNode>& Node,
		bool bIsInput,
		TSharedPtr<SWidget>& OutItemsNumWidget,
		TSharedPtr<SWidget>& OutButtonsWidget,
		TSharedPtr<SWidget>& OutNoEntriesWidget) const;

	void CreateAdvancedCategory(const TSharedPtr<SVerticalBox>& MainBox) const;

	TSharedRef<SWidget> MakeStatWidget() const;

	EVisibility GetButtonVisibility(bool bVisible) const;
	void CreateStandardNodeTitleButtons(const TSharedPtr<SHorizontalBox>& TitleBox);

	TArray<FOverlayWidget> OverlayWidgets;

	TMap<FName, TArray<TWeakPtr<SGraphPin>>> CategoryToChildPins;
	bool bCreateAdvancedCategory = false;

protected:
	TSharedPtr<IVoxelNodeDefinition> NodeDefinition;

	// Keep nodes alive so we can check WeakParent
	TSharedPtr<const IVoxelNodeDefinition::FNode> Inputs;
	TSharedPtr<const IVoxelNodeDefinition::FNode> Outputs;
};