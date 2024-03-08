// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Curves/KeyHandle.h"
#include "VoxelCurveDragOperation.h"

class VOXELGRAPHEDITOR_API FVoxelCurveDragOperation_Marquee : public FVoxelCurveDragOperation
{
public:
	using FVoxelCurveDragOperation::FVoxelCurveDragOperation;

private:
	//~ Begin FVoxelCurveDragOperation Interface
	virtual void OnDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnPaint(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 PaintOnLayerId) override;
	virtual void OnEndDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnCancel(const FPointerEvent& MouseEvent) override;
	virtual bool CancelOnMouseLeave() const override { return false; }
	//~ End FVoxelCurveDragOperation Interface

private:
	FSlateRect MarqueeRect;
	FVector2D TopLeft = FVector2D::ZeroVector;
	FVector2D BottomRight = FVector2D::ZeroVector;
	TSet<FKeyHandle> CachedSelectedKeys;
};