// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCurveDragOperation.h"
#include "CurveEditor/SVoxelCurvePanel.h"

FVoxelCurveDragOperation::FVoxelCurveDragOperation(const TSharedPtr<SVoxelCurvePanel>& Panel, const TSharedPtr<FVoxelCurveModel>& CurveModel, const FVector2D InitialPosition, const FKey& EffectiveKey)
	: EffectiveKey(EffectiveKey)
	, WeakPanel(Panel)
	, WeakCurveModel(CurveModel)
	, InitialPosition(InitialPosition)
{
}

FReply FVoxelCurveDragOperation::OnMouseMove(const FVector2D& Position, const FPointerEvent& MouseEvent)
{
	if (bIsDragging)
	{
		CurrentPosition = Position;
		OnDrag(MouseEvent);
		return FReply::Handled();
	}

	if (!AttemptDragStart(Position, MouseEvent))
	{
		return FReply::Unhandled();
	}

	DragInitialPosition = Position;
	CurrentPosition = Position;
	OnBeginDrag(MouseEvent);

	return FReply::Handled().CaptureMouse(WeakPanel.Pin().ToSharedRef());
}

void FVoxelCurveDragOperation::OnMouseUp(const FVector2D& Position, const FPointerEvent& MouseEvent)
{
	if (!bIsDragging)
	{
		return;
	}

	CurrentPosition = Position;
	OnEndDrag(MouseEvent);
}

TSharedPtr<SVoxelCurvePanel> FVoxelCurveDragOperation::GetPanel() const
{
	return WeakPanel.Pin();
}

TSharedPtr<FVoxelCurveModel> FVoxelCurveDragOperation::GetCurveModel() const
{
	return WeakCurveModel.Pin();
}

bool FVoxelCurveDragOperation::AttemptDragStart(const FVector2D Position, const FPointerEvent& MouseEvent)
{
	if (!bIsDragging &&
		MouseEvent.IsMouseButtonDown(EffectiveKey))
	{
		DistanceDragged += MouseEvent.GetCursorDelta().Size();
		if (DistanceDragged > FMath::Max(FSlateApplication::Get().GetDragTriggerDistance() * 0.1f, 1.f))
		{
			DragInitialPosition = Position;
			bIsDragging = true;
			return true;
		}
	}
	return false;
}