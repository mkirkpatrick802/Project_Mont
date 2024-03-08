// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class SVoxelCurvePanel;
class FVoxelCurveModel;

class VOXELGRAPHEDITOR_API FVoxelCurveDragOperation
{
private:
	bool bIsDragging = false;
	float DistanceDragged = 0.f;
	FKey EffectiveKey;

protected:
	TWeakPtr<SVoxelCurvePanel> WeakPanel;
	TWeakPtr<FVoxelCurveModel> WeakCurveModel;
	FVector2D InitialPosition = FVector2D::ZeroVector;
	FVector2D DragInitialPosition = FVector2D::ZeroVector;
	FVector2D CurrentPosition;

public:
	explicit FVoxelCurveDragOperation(const TSharedPtr<SVoxelCurvePanel>& Panel, const TSharedPtr<FVoxelCurveModel>& CurveModel, FVector2D InitialPosition, const FKey& EffectiveKey);

	bool IsDragging() const { return bIsDragging; }

	FReply OnMouseMove(const FVector2D& Position, const FPointerEvent& MouseEvent);
	void OnMouseUp(const FVector2D& Position, const FPointerEvent& MouseEvent);

protected:
	TSharedPtr<SVoxelCurvePanel> GetPanel() const;
	TSharedPtr<FVoxelCurveModel> GetCurveModel() const;

private:
	bool AttemptDragStart(FVector2D Position, const FPointerEvent& MouseEvent);

	virtual void OnBeginDrag(const FPointerEvent& MouseEvent) {}
	virtual void OnDrag(const FPointerEvent& MouseEvent) {}
	virtual void OnEndDrag(const FPointerEvent& MouseEvent) {}

public:
	virtual void OnPaint(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 PaintOnLayerId) {}
	virtual void OnCancel(const FPointerEvent& MouseEvent) {}
	virtual bool CancelOnMouseLeave() const { return false; }

	virtual ~FVoxelCurveDragOperation() = default;
};