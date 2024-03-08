// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "CurveDataAbstraction.h"
#include "VoxelCurveEditorData.h"

class FVoxelCurveModel;
class FVoxelCurveDrag;
class FVoxelCurveDragOperation;
struct FVoxelCurveGridSpacing;

class VOXELGRAPHEDITOR_API SVoxelCurvePanel : public SLeafWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(TSharedPtr<FVoxelCurveModel>, CurveModel)
		SLATE_ARGUMENT(TSharedPtr<FUICommandList>, CommandList)
		SLATE_ARGUMENT(FLinearColor, CurveTint)
		SLATE_ARGUMENT(FLinearColor, KeysTint)
		SLATE_ARGUMENT(FLinearColor, TangentsTint)

		SLATE_ATTRIBUTE(float, MaxHeight)
		SLATE_EVENT(FSimpleDelegate, Invalidate)
	};

	void Construct(const FArguments& InArgs);

	//~ Begin SCompoundWidget Interface
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual bool IsInteractable() const override { return true; }
	//~ End SCompoundWidget Interface

	void BindCommands();

private:
	void DrawGridLines(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 BaseLayerId,
		ESlateDrawEffect DrawEffects) const;

	void DrawCurve(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 BaseLayerId,
		ESlateDrawEffect DrawEffects) const;

private:
	bool FindClosestPoint(const FVector2D& MousePixel);
	void CreateContextMenu(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

public:
	void ToggleInputSnapping() { bInputSnappingEnabled = !bInputSnappingEnabled; }
	bool IsInputSnappingEnabled() const { return bInputSnappingEnabled; }

	void ToggleOutputSnapping() { bOutputSnappingEnabled = !bOutputSnappingEnabled; }
	bool IsOutputSnappingEnabled() const { return bOutputSnappingEnabled; }

	TOptional<float> GetGridSpacing(const bool bInput) const { return bInput ? InputGridSpacing : OutputGridSpacing; }
	void SetGridSpacing(TOptional<float> NewGridSpacing, bool bInput);

	bool CompareAxisSnapping(const EAxisList::Type Axis) const { return AxisSnapping == Axis; }
	EAxisList::Type GetAxisSnapping() const { return AxisSnapping; }
	void SetAxisSnapping(const EAxisList::Type Snap) { AxisSnapping = Snap; }

	TOptional<FVoxelCurveKeyPoint> GetHoveredKeyHandle() const { return HoveredKeyHandle; }

private:
	TSharedPtr<FVoxelCurveModel> CurveModel;

	FLinearColor CurveTint;
	FLinearColor KeysTint;
	FLinearColor TangentsTint;

	bool bInputSnappingEnabled = true;
	bool bOutputSnappingEnabled = true;
	TOptional<float> InputGridSpacing;
	TOptional<float> OutputGridSpacing;
	EAxisList::Type AxisSnapping = EAxisList::None;

	TOptional<FVoxelCurveKeyPoint> HoveredKeyHandle;

private:
	TOptional<float> HoveredInput;
	TAttribute<float> MaxHeight;
	TSharedPtr<FVoxelCurveDragOperation> DragOperation;
	FSimpleDelegate Invalidate;

	TWeakPtr<IMenu> ActiveContextMenu;

	FVector2D CachedMousePosition;

	TSharedPtr<FUICommandList> CommandList;

	FKeyAttributes CachedKeyAttributes;
	TOptional<FVoxelCachedToolTipData> CachedToolTip;

private:
	friend class FVoxelCurveModel;
};