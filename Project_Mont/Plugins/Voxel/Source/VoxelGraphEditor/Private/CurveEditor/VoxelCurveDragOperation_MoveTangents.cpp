// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCurveDragOperation_MoveTangents.h"
#include "SVoxelCurvePanel.h"
#include "VoxelCurveEditorUtilities.h"
#include "VoxelCurveModel.h"

FVoxelCurveDragOperation_MoveTangents::FVoxelCurveDragOperation_MoveTangents(const TSharedPtr<SVoxelCurvePanel>& Panel, const TSharedPtr<FVoxelCurveModel>& CurveModel, const FVector2D InitialPosition, const FKey& EffectiveKey, const FVoxelCurveKeyPoint& MovedTangent)
	: FVoxelCurveDragOperation(Panel, CurveModel, InitialPosition, EffectiveKey)
	, bMovingArriveTangent(MovedTangent.PointType == FVoxelCurveKeyPoint::EKeyPointType::ArriveTangent)
{
	const FKeyHandle FirstKeyHandle = CurveModel->Curve->GetFirstKeyHandle();
	const FKeyHandle LastKeyHandle = CurveModel->Curve->GetLastKeyHandle();

	for (const FKeyHandle& KeyHandle : CurveModel->SelectedKeys)
	{
		const FRichCurveKey& CurveKey = CurveModel->Curve->GetKey(KeyHandle);
		if (CurveKey.InterpMode == RCIM_Constant ||
			CurveKey.InterpMode == RCIM_Linear)
		{
			continue;
		}

		if (MovedTangent.PointType == FVoxelCurveKeyPoint::EKeyPointType::ArriveTangent)
		{
			if (KeyHandle != FirstKeyHandle)
			{
				Keys.Add({ KeyHandle, CurveKey.Time, CurveKey.Value, CurveKey.ArriveTangent });
			}
		}
		else
		{
			if (KeyHandle != LastKeyHandle)
			{
				Keys.Add({ KeyHandle, CurveKey.Time, CurveKey.Value, CurveKey.LeaveTangent });
			}
		}
	}
}

void FVoxelCurveDragOperation_MoveTangents::OnBeginDrag(const FPointerEvent& MouseEvent)
{
	Transaction = MakeUnique<FScopedTransaction>(INVTEXT("Move Tangents"));
}

void FVoxelCurveDragOperation_MoveTangents::OnDrag(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel)
	{
		return;
	}

	const float DisplayRatio = CurveModel->GetPixelsPerOutput() / CurveModel->GetPixelsPerInput();

	const FVector2D PixelDelta = CurrentPosition - InitialPosition;

	const float CrossoverThreshold = bMovingArriveTangent ? -1.f : 1.f;

	using FUpdateTangentType = TFunction<void(FRichCurveKey& Key, FVector2D& TangentOffset)>;
	const FUpdateTangentType UpdateArriveTangent = [&](FRichCurveKey& Key, FVector2D& TangentOffset)
	{
		TangentOffset.X = FMath::Min(TangentOffset.X, CrossoverThreshold);

		const float Tangent = (-TangentOffset.Y / TangentOffset.X) / DisplayRatio;

		FKeyAttributes Attributes;
		Attributes.SetArriveTangent(Tangent);
		FVoxelCurveEditorUtilities::ApplyAttributesToKey(Key, Attributes);
	};
	const FUpdateTangentType UpdateLeaveTangent = [&](FRichCurveKey& Key, FVector2D& TangentOffset)
	{
		TangentOffset.X = FMath::Max(TangentOffset.X, CrossoverThreshold);

		const float Tangent = (-TangentOffset.Y / TangentOffset.X) / DisplayRatio;

		FKeyAttributes Attributes;
		Attributes.SetLeaveTangent(Tangent);
		FVoxelCurveEditorUtilities::ApplyAttributesToKey(Key, Attributes);
	};

	const FUpdateTangentType UpdateTangent = bMovingArriveTangent ? UpdateArriveTangent : UpdateLeaveTangent;
	const float PixelLength = bMovingArriveTangent ? -60.0f : 60.f;

	for (const FKeyData& Key : Keys)
	{
		if (!ensure(CurveModel->Curve->IsKeyHandleValid(Key.Handle)))
		{
			continue;
		}

		FVector2D TangentOffset = FVoxelCurveEditorUtilities::GetVectorFromSlopeAndLength(Key.Tangent * -DisplayRatio, PixelLength);
		TangentOffset += PixelDelta;

		if (MouseEvent.IsShiftDown())
		{
			TangentOffset = RoundTrajectory(TangentOffset);
		}

		UpdateTangent(CurveModel->Curve->GetKey(Key.Handle), TangentOffset);
	}

	CurveModel->UpdateCachedKeyAttributes();
}

void FVoxelCurveDragOperation_MoveTangents::OnEndDrag(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel)
	{
		return;
	}

	CurveModel->UpdatePanelRanges();
	CurveModel->SaveCurve();
}

void FVoxelCurveDragOperation_MoveTangents::OnCancel(const FPointerEvent& MouseEvent)
{
	const TSharedPtr<FVoxelCurveModel> CurveModel = GetCurveModel();
	if (!CurveModel)
	{
		return;
	}

	CurveModel->UpdatePanelRanges();
	CurveModel->SaveCurve();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVector2D FVoxelCurveDragOperation_MoveTangents::RoundTrajectory(const FVector2D& Delta)
{
	const float Distance = Delta.Size();
	float Theta = FMath::Atan2(Delta.Y, Delta.X) + HALF_PI;

	constexpr float RoundTo = PI / 4;
	Theta = FMath::RoundToInt(Theta / RoundTo) * RoundTo - HALF_PI;

	return FVector2D(Distance * FMath::Cos(Theta), Distance * FMath::Sin(Theta));
}