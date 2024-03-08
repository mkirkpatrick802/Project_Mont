// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCurveDragOperation_MoveKeys.h"
#include "SVoxelCurvePanel.h"
#include "VoxelCurveModel.h"

FVoxelCurveDragOperation_MoveKeys::FVoxelCurveDragOperation_MoveKeys(const TSharedPtr<SVoxelCurvePanel>& Panel, const TSharedPtr<FVoxelCurveModel>& CurveModel, const FVector2D InitialPosition, const FKey& EffectiveKey)
	: FVoxelCurveDragOperation(Panel, CurveModel, InitialPosition, EffectiveKey)
{
	for (const FKeyHandle& KeyHandle : CurveModel->SelectedKeys)
	{
		const FRichCurveKey& CurveKey = CurveModel->Curve->GetKey(KeyHandle);
		Keys.Add({ KeyHandle, CurveKey.Time, CurveKey.Value });
	}
}

void FVoxelCurveDragOperation_MoveKeys::OnBeginDrag(const FPointerEvent& MouseEvent)
{
	LastMousePosition = CurrentPosition;
	Transaction = MakeUnique<FScopedTransaction>(INVTEXT("Move Keys"));
}

void FVoxelCurveDragOperation_MoveKeys::OnDrag(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<SVoxelCurvePanel> Panel = GetPanel();
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel ||
		!CurveModel->Curve)
	{
		return;
	}

	ON_SCOPE_EXIT
	{
		LastMousePosition = CurrentPosition;
	};

	const float DeltaInput = (CurrentPosition.X - DragInitialPosition.X) / CurveModel->GetPixelsPerInput();
	const float DeltaOutput = -(CurrentPosition.Y - DragInitialPosition.Y) / CurveModel->GetPixelsPerOutput();

	const FSlateRect Dimensions = CurveModel->GetSnapDimensions();
	const FVector2D SnapIntervals = CurveModel->GetSnapIntervals();

	for (const FKeyData& Key : Keys)
	{
		float TimeValue = Key.Time + DeltaInput;
		if (Panel->IsInputSnappingEnabled())
		{
			const int32 TimeMultiplier = FMath::RoundHalfToEven((Key.Time + DeltaInput) / SnapIntervals.X);
			TimeValue = TimeMultiplier * SnapIntervals.X;
		}

		float OutputValue = Key.Value + DeltaOutput;
		if (Panel->IsOutputSnappingEnabled())
		{
			const int32 ValueMultiplier = FMath::RoundHalfToEven((Key.Value + DeltaOutput) / SnapIntervals.Y);
			OutputValue = ValueMultiplier * SnapIntervals.Y;
		}

		switch (Panel->GetAxisSnapping())
		{
		case EAxisList::X: CurveModel->Curve->SetKeyTime(Key.Key, FMath::Clamp(TimeValue, Dimensions.Left, Dimensions.Right)); break;
		case EAxisList::Y: CurveModel->Curve->GetKey(Key.Key).Value = FMath::Clamp(OutputValue, Dimensions.Top, Dimensions.Bottom); break;
		default:
		{
			CurveModel->Curve->GetKey(Key.Key).Value = FMath::Clamp(OutputValue, Dimensions.Top, Dimensions.Bottom);
			CurveModel->Curve->SetKeyTime(Key.Key, FMath::Clamp(TimeValue, Dimensions.Left, Dimensions.Right));
			break;
		}
		}
	}

	CurveModel->UpdateCachedKeyAttributes();
}

void FVoxelCurveDragOperation_MoveKeys::OnEndDrag(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel)
	{
		return;
	}

	CurveModel->UpdatePanelRanges();
	CurveModel->SaveCurve();
}

void FVoxelCurveDragOperation_MoveKeys::OnCancel(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	const TSharedPtr<SVoxelCurvePanel> Panel = GetPanel();
	if (!CurveModel ||
		!Panel)
	{
		return;
	}

	if (!MouseEvent.IsControlDown() &&
		Panel->GetHoveredKeyHandle().IsSet())
	{
		CurveModel->SelectKey(Panel->GetHoveredKeyHandle()->Handle);
	}

	CurveModel->UpdatePanelRanges();
	CurveModel->SaveCurve();
}