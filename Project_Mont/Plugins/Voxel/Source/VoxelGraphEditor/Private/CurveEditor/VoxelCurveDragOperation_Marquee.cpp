// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCurveDragOperation_Marquee.h"
#include "VoxelCurveModel.h"

void FVoxelCurveDragOperation_Marquee::OnDrag(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel)
	{
		return;
	}

	MarqueeRect.Left = FMath::Max(FMath::Min(DragInitialPosition.X, CurrentPosition.X), CurveModel->CachedCurveScreenBox.Left);
	MarqueeRect.Top = FMath::Max(FMath::Min(DragInitialPosition.Y, CurrentPosition.Y), CurveModel->CachedCurveScreenBox.Top);

	MarqueeRect.Right = FMath::Min(FMath::Max(DragInitialPosition.X, CurrentPosition.X), CurveModel->CachedCurveScreenBox.Right);
	MarqueeRect.Bottom = FMath::Min(FMath::Max(DragInitialPosition.Y, CurrentPosition.Y), CurveModel->CachedCurveScreenBox.Bottom);

	TSet<FKeyHandle> SelectedKeys;
	bool bAllKeysExist = true;
	for (const FVoxelCurveKeyPoint& Key : CurveModel->CachedKeyPoints)
	{
		if (Key.PointType != FVoxelCurveKeyPoint::EKeyPointType::None)
		{
			continue;
		}

		if (MarqueeRect.ContainsPoint(Key.ScreenPosition))
		{
			SelectedKeys.Add(Key.Handle);
			bAllKeysExist &= CachedSelectedKeys.Contains(Key.Handle);
		}
	}

	if (!bAllKeysExist ||
		CachedSelectedKeys.Num() != SelectedKeys.Num())
	{
		CurveModel->SelectKeys(SelectedKeys);
		CachedSelectedKeys = SelectedKeys;
	}
}

void FVoxelCurveDragOperation_Marquee::OnPaint(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, const int32 PaintOnLayerId)
{
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		PaintOnLayerId + 55,
		AllottedGeometry.ToPaintGeometry(FVector2D(MarqueeRect.Right - MarqueeRect.Left, MarqueeRect.Bottom - MarqueeRect.Top), FSlateLayoutTransform(FVector2D(MarqueeRect.Left, MarqueeRect.Top))),
		FAppStyle::GetBrush(TEXT("MarqueeSelection")));
}

void FVoxelCurveDragOperation_Marquee::OnEndDrag(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel)
	{
		return;
	}

	CurveModel->OnInvalidate.ExecuteIfBound();
}

void FVoxelCurveDragOperation_Marquee::OnCancel(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel)
	{
		return;
	}

	CurveModel->OnInvalidate.ExecuteIfBound();
}