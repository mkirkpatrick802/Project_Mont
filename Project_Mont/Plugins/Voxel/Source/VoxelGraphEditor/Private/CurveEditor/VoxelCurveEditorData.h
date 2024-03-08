// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Curves/RichCurve.h"

struct FVoxelCurveGridSpacing
{
	struct FSlice
	{
		float Value;
		bool bIsMajor;
		FText Label;
		FVector2D LabelSize;
		FSlice(const double Value, const bool bIsMajor, const FText& Label, const FVector2D& LabelSize)
			: Value(Value)
			, bIsMajor(bIsMajor)
			, Label(Label)
			, LabelSize(LabelSize)
		{}
		FSlice(const double Value, const bool bIsMajor)
			: Value(Value)
			, bIsMajor(bIsMajor)
		{}
	};

	TArray<FSlice> Slices;
	FVector2D MaxLabelSize = FVector2D::ZeroVector;
	FVector2D LastLabelSize = FVector2D::ZeroVector;
	float SliceInterval = 0.f;
};

struct FVoxelCachedToolTipData
{
	FText InputText;
	FText OutputText;

	FVoxelCachedToolTipData(const float Time, const float Value)
		: InputText(FText::FromString("X: " + FText::AsNumber(Time).ToString()))
		, OutputText(FText::FromString("Y: " + FText::AsNumber(Value).ToString()))
	{}
};

struct FVoxelCurveKeyPoint
{
	enum class EKeyPointType
	{
		None,
		ArriveTangent,
		LeaveTangent
	};

	FKeyHandle Handle;
	FVector2D ScreenPosition;
	FVector2D BrushSize;
	const FSlateBrush* Brush = nullptr;
	int32 LayerBias;
	FVector2D LineDelta = FVector2D::ZeroVector;
	EKeyPointType PointType;

	FVoxelCurveKeyPoint(const FVector2D& PointScreenPosition, const FVector2D& Delta, const FKeyHandle& Handle, const bool bArriveTangent)
		: Handle(Handle)
		, ScreenPosition(PointScreenPosition + Delta)
		, BrushSize(FVector2D(9.f))
		, Brush(FAppStyle::GetBrush("GenericCurveEditor.TangentHandle"))
		, LayerBias(1)
		, LineDelta(PointScreenPosition - (PointScreenPosition + Delta))
		, PointType(bArriveTangent ? EKeyPointType::ArriveTangent : EKeyPointType::LeaveTangent)
	{
	}
	FVoxelCurveKeyPoint(const FVector2D& ScreenPosition, const ERichCurveInterpMode KeyType, const FKeyHandle& Handle)
		: Handle(Handle)
		, ScreenPosition(ScreenPosition)
		, BrushSize(FVector2D(11.f))
		, LayerBias(2)
		, PointType(EKeyPointType::None)
	{
		switch (KeyType)
		{
		case RCIM_Constant: Brush = FAppStyle::GetBrush("GenericCurveEditor.ConstantKey"); break;
		case RCIM_Linear: Brush = FAppStyle::GetBrush("GenericCurveEditor.LinearKey"); break;
		case RCIM_Cubic: Brush = FAppStyle::GetBrush("GenericCurveEditor.CubicKey"); break;
		default: Brush = FAppStyle::GetBrush("GenericCurveEditor.Key"); break;
		}
	}
};