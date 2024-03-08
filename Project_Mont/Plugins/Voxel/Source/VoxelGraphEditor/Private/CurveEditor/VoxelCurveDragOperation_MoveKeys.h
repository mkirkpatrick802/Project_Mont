// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Curves/KeyHandle.h"
#include "VoxelCurveDragOperation.h"

class VOXELGRAPHEDITOR_API FVoxelCurveDragOperation_MoveKeys : public FVoxelCurveDragOperation
{
public:
	explicit FVoxelCurveDragOperation_MoveKeys(const TSharedPtr<SVoxelCurvePanel>& Panel, const TSharedPtr<FVoxelCurveModel>& CurveModel, FVector2D InitialPosition, const FKey& EffectiveKey);

private:
	struct FKeyData
	{
		FKeyHandle Key;
		double Time;
		double Value;
	};

private:
	//~ Begin FVoxelCurveDragOperation Interface
	virtual void OnBeginDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnEndDrag(const FPointerEvent& MouseEvent) override;
	virtual void OnCancel(const FPointerEvent& MouseEvent) override;
	//~ End FVoxelCurveDragOperation Interface

private:
	TArray<FKeyData> Keys;
	FVector2D LastMousePosition;
	TUniquePtr<FScopedTransaction> Transaction;
};