// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCurveEditorUtilities.h"
#include "Fonts/FontMeasure.h"
#include "Widgets/SVoxelCurveKeySelector.h"

void FVoxelCurveEditorUtilities::CalculateFixedGridSpacing(
	const float GridSpacing,
	const FVector2D& ValueRange,
	FVoxelCurveGridSpacing& OutSpacingData)
{
	OutSpacingData = {};
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const double MajorGridStep = GridSpacing * 4;
	const double FirstMinorLine = FMath::FloorToDouble(ValueRange.X / GridSpacing) * GridSpacing;
	const double LastMinorLine = FMath::CeilToDouble(ValueRange.Y / GridSpacing) * GridSpacing;

	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.SetMaximumFractionalDigits(6);

	const FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont");
	const TSharedRef<FSlateFontMeasure> FontMeasureService = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();

	OutSpacingData.SliceInterval = GridSpacing;

	OutSpacingData.Slices.Add(MakeMajorSlice(FontMeasure, FontInfo, ValueRange.X, OutSpacingData));

	for (double CurrentMinorLine = FirstMinorLine; CurrentMinorLine <= LastMinorLine; CurrentMinorLine += GridSpacing)
	{
		if (CurrentMinorLine <= ValueRange.X ||
			CurrentMinorLine >= ValueRange.Y)
		{
			continue;
		}

		if (FMath::IsNearlyZero(FMath::Fmod(FMath::Abs(CurrentMinorLine), MajorGridStep)))
		{
			OutSpacingData.Slices.Add(MakeMajorSlice(FontMeasure, FontInfo, CurrentMinorLine, OutSpacingData));
		}
		else
		{
			OutSpacingData.Slices.Add({ CurrentMinorLine, false });
		}
	}

	OutSpacingData.Slices.Add(MakeMajorSlice(FontMeasure, FontInfo, ValueRange.Y, OutSpacingData));
}

void FVoxelCurveEditorUtilities::CalculateAutomaticGridSpacing(
	const float Size,
	const FVector2D& ValueRange,
	const float IntervalWidth,
	FVoxelCurveGridSpacing& OutSpacingData)
{
	OutSpacingData = {};
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont");

	const float Range = FMath::Max(ValueRange.Y - ValueRange.X, 0.00001f);
	const float GridPixelSpacing = Size / 5.f;
	const float PixelsPerInput = (Size / Range);
	const float Order = FMath::Pow(10.f, FMath::FloorToInt(FMath::LogX(10.f, GridPixelSpacing / PixelsPerInput)));

	static constexpr int32 DesirableBases[]  = { 2, 5 };
	static constexpr int32 NumDesirableBases = UE_ARRAY_COUNT(DesirableBases);

	const int32 Scale = FMath::RoundToInt(GridPixelSpacing / PixelsPerInput / Order);
	int32 Base = DesirableBases[0];
	for (int32 BaseIndex = 1; BaseIndex < NumDesirableBases; ++BaseIndex)
	{
		if (FMath::Abs(Scale - DesirableBases[BaseIndex]) < FMath::Abs(Scale - Base))
		{
			Base = DesirableBases[BaseIndex];
		}
	}

	const double MajorInterval = FMath::Pow(float(Base), FMath::FloorToFloat(FMath::LogX(float(Base), float(Scale)))) * Order;

	const double FirstMajorLine = FMath::FloorToDouble(ValueRange.X / MajorInterval) * MajorInterval;
	const double LastMajorLine = FMath::CeilToDouble(ValueRange.Y / MajorInterval) * MajorInterval;

	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.SetMaximumFractionalDigits(6);

	const float MajorIntervalWidth = (MajorInterval / Range) * Size;
	const int32 MinorDivisions = FMath::Min(1 << FMath::CeilLogTwo(FMath::FloorToInt32(MajorIntervalWidth / IntervalWidth)), 4);

	OutSpacingData.SliceInterval = MajorInterval / MinorDivisions;

	OutSpacingData.Slices.Add(MakeMajorSlice(FontMeasure, FontInfo, ValueRange.X, OutSpacingData));

	for (double CurrentMajorLine = FirstMajorLine; CurrentMajorLine <= LastMajorLine; CurrentMajorLine += MajorInterval)
	{
		if (CurrentMajorLine > ValueRange.X &&
			CurrentMajorLine < ValueRange.Y)
		{
			OutSpacingData.Slices.Add(MakeMajorSlice(FontMeasure, FontInfo, CurrentMajorLine, OutSpacingData));
		}

		for (int32 Step = 1; Step < MinorDivisions; ++Step)
		{
			const float MinorLine = CurrentMajorLine + (Step * MajorInterval / MinorDivisions);
			if (MinorLine <= ValueRange.X ||
				MinorLine >= ValueRange.Y)
			{
				continue;
			}

			OutSpacingData.Slices.Add({ MinorLine, false });
		}
	}

	OutSpacingData.Slices.Add(MakeMajorSlice(FontMeasure, FontInfo, ValueRange.Y, OutSpacingData));
}

FVector2D FVoxelCurveEditorUtilities::GetVectorFromSlopeAndLength(const float Slope, const float Length)
{
	const float X = Length / FMath::Sqrt(Slope * Slope + 1.f);
	const float Y = Slope * X;
	return FVector2D(X, Y);
}

void FVoxelCurveEditorUtilities::ConvertPointToScreenSpace(FVector2D& Point, const FVector2D& CurveMin, const FVector2D& CurveMax, const FSlateRect& ScreenBox)
{
	Point.X = (Point.X - CurveMin.X) / (CurveMax.X - CurveMin.X);
	Point.X = Point.X * (ScreenBox.Right - ScreenBox.Left) + ScreenBox.Left;
	Point.Y = (Point.Y - CurveMin.Y) / (CurveMax.Y - CurveMin.Y);
	Point.Y = (1.f - Point.Y) * (ScreenBox.Bottom - ScreenBox.Top) + ScreenBox.Top;
}

void FVoxelCurveEditorUtilities::RefineCurvePoints(
	const TSharedPtr<FRichCurve>& RichCurve,
	const FSlateRect& Size,
	const float MinOutput,
	const float MaxOutput,
	TArray<FVector2D>& OutPoints)
{
	const float Width = Size.Right - Size.Left;
	const float Height = Size.Bottom - Size.Top;

	const double TimeThreshold = FMath::Max(0.0001f, 1.0 / Width);
	const double ValueThreshold = FMath::Max(0.0001f, 1.0 / Height);

	constexpr int32 NumInterpTimes = 3;
	constexpr float InterpTimes[] = { 0.25f, 0.5f, 0.6f };

	for (int32 Index = 0; Index < OutPoints.Num() - 1; Index++)
	{
		FVector2D Lower = OutPoints[Index];
		FVector2D Upper = OutPoints[Index + 1];

		if ((Upper.X - Lower.X) < TimeThreshold)
		{
			continue;
		}

		bool bSegmentIsLinear = true;

		FVector2D Evaluated[NumInterpTimes] = { FVector2D::ZeroVector, FVector2D::ZeroVector, FVector2D::ZeroVector };
		for (int32 InterpIndex = 0; InterpIndex < NumInterpTimes; InterpIndex++)
		{
			Evaluated[InterpIndex].X = FMath::Lerp(Lower.X, Upper.X, InterpTimes[InterpIndex]);

			const float Value = RichCurve->Eval(Evaluated[InterpIndex].X);
			Evaluated[InterpIndex].Y = FMath::Clamp(Value, MinOutput, MaxOutput);

			const float LinearValue = FMath::Lerp(Lower.Y, Upper.Y, InterpTimes[InterpIndex]);
			bSegmentIsLinear &= FMath::IsNearlyEqual(Value, LinearValue, ValueThreshold);
		}

		if (bSegmentIsLinear)
		{
			continue;
		}

		OutPoints.Insert(Evaluated, NumInterpTimes, Index + 1);
		--Index;
	}
}

FKeyAttributes FVoxelCurveEditorUtilities::MakeKeyAttributesFromHandle(const FRichCurve& Curve, const FKeyHandle& KeyHandle)
{
	FKeyAttributes Result;

	if (!Curve.IsKeyHandleValid(KeyHandle))
	{
		return Result;
	}

	const FRichCurveKey& Key = Curve.GetKey(KeyHandle);

	Result.SetInterpMode(Key.InterpMode);

	if (Key.InterpMode != RCIM_Constant &&
		Key.InterpMode != RCIM_Linear)
	{
		Result.SetTangentMode(Key.TangentMode);

		if (Curve.GetFirstKeyHandle() != KeyHandle)
		{
			Result.SetArriveTangent(Key.ArriveTangent);
		}
		if (Curve.GetLastKeyHandle() != KeyHandle)
		{
			Result.SetLeaveTangent(Key.LeaveTangent);
		}
	}

	return Result;
}

bool FVoxelCurveEditorUtilities::ApplyAttributesToKey(FRichCurveKey& Key, const FKeyAttributes& Attributes)
{
	bool bAutoSetTangents = false;

	if (Attributes.HasInterpMode())
	{
		Key.InterpMode = Attributes.GetInterpMode();
		bAutoSetTangents = true;
	}

	if (Attributes.HasTangentMode())
	{
		Key.TangentMode = Attributes.GetTangentMode();
		if (Key.TangentMode == RCTM_Auto)
		{
			Key.TangentWeightMode = RCTWM_WeightedNone;
		}

		bAutoSetTangents = true;
	}

	if (Attributes.HasArriveTangent())
	{
		if (Key.TangentMode == RCTM_Auto)
		{
			Key.TangentMode = RCTM_User;
			Key.TangentWeightMode = RCTWM_WeightedNone;
		}

		Key.ArriveTangent = Attributes.GetArriveTangent();
		if (Key.InterpMode == RCIM_Cubic &&
			Key.TangentMode != RCTM_Break)
		{
			Key.LeaveTangent = Key.ArriveTangent;
		}
	}

	if (Attributes.HasLeaveTangent())
	{
		if (Key.TangentMode == RCTM_Auto)
		{
			Key.TangentMode = RCTM_User;
			Key.TangentWeightMode = RCTWM_WeightedNone;
		}

		Key.LeaveTangent = Attributes.GetLeaveTangent();
		if (Key.InterpMode == RCIM_Cubic &&
			Key.TangentMode != RCTM_Break)
		{
			Key.ArriveTangent = Key.LeaveTangent;
		}
	}

	return bAutoSetTangents;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelCurveGridSpacing::FSlice FVoxelCurveEditorUtilities::MakeMajorSlice(
	const TSharedRef<FSlateFontMeasure>& FontMeasure,
	const FSlateFontInfo& FontInfo,
	const float LineValue,
	FVoxelCurveGridSpacing& OutSpacingData)
{
	float Value = LineValue;
	FText Label = FText::AsNumber(LineValue);
	const FVector2D LabelSize = FontMeasure->Measure(Label, FontInfo);

	OutSpacingData.MaxLabelSize = FVector2D::Max(OutSpacingData.MaxLabelSize, LabelSize);
	OutSpacingData.LastLabelSize = LabelSize;
	return { Value , true, Label, LabelSize };
}