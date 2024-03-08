// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelCurveEditorData.h"

struct FKeyAttributes;

class FVoxelCurveEditorUtilities
{
public:
	static void CalculateFixedGridSpacing(
		float GridSpacing,
		const FVector2D& ValueRange,
		FVoxelCurveGridSpacing& OutSpacingData);

	static void CalculateAutomaticGridSpacing(
		float Size,
		const FVector2D& ValueRange,
		float IntervalWidth,
		FVoxelCurveGridSpacing& OutSpacingData);

	static FVector2D GetVectorFromSlopeAndLength(float Slope, float Length);
	static void ConvertPointToScreenSpace(FVector2D& Point, const FVector2D& CurveMin, const FVector2D& CurveMax, const FSlateRect& ScreenBox);
	static void RefineCurvePoints(
		const TSharedPtr<FRichCurve>& RichCurve,
		const FSlateRect& Size,
		float MinOutput,
		float MaxOutput,
		TArray<FVector2D>& OutPoints);

	static FKeyAttributes MakeKeyAttributesFromHandle(const FRichCurve& Curve, const FKeyHandle& KeyHandle);
	static bool ApplyAttributesToKey(FRichCurveKey& Key, const FKeyAttributes& Attributes);

private:
	static FVoxelCurveGridSpacing::FSlice MakeMajorSlice(
		const TSharedRef<FSlateFontMeasure>& FontMeasure,
		const FSlateFontInfo& FontInfo,
		float LineValue,
		FVoxelCurveGridSpacing& OutSpacingData);
};