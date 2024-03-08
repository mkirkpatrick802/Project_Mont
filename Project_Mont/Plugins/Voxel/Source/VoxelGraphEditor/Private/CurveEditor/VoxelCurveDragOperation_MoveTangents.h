// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SVoxelCurvePanel.h"
#include "VoxelCurveDragOperation.h"

class VOXELGRAPHEDITOR_API FVoxelCurveDragOperation_MoveTangents : public FVoxelCurveDragOperation
{
public:
	explicit FVoxelCurveDragOperation_MoveTangents(
		const TSharedPtr<SVoxelCurvePanel>& Panel,
		const TSharedPtr<FVoxelCurveModel>& CurveModel,
		FVector2D InitialPosition,
		const FKey& EffectiveKey,
		const FVoxelCurveKeyPoint& MovedTangent);

private:
	struct FKeyData
	{
		FKeyHandle Handle;
		double Time;
		double Value;
		float Tangent;
	};

private:
	//~ Begin FVoxelCurveDragOperation Interface
	virtual void OnBeginDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnEndDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnCancel(const FPointerEvent& MouseEvent) override;
	//~ End FVoxelCurveDragOperation Interface

	static FVector2D RoundTrajectory(const FVector2D& Delta);

private:
	TArray<FKeyData> Keys;
	FVector2D LastMousePosition;
	bool bMovingArriveTangent;
	TUniquePtr<FScopedTransaction> Transaction;
};