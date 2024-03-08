// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelCurvePanel.h"

#include "VoxelCurveModel.h"
#include "CurveEditor/VoxelCurveDragOperation.h"
#include "CurveEditor/VoxelCurveDragOperation_Marquee.h"
#include "CurveEditor/VoxelCurveDragOperation_MoveKeys.h"
#include "CurveEditor/VoxelCurveDragOperation_MoveTangents.h"

#include "Fonts/FontMeasure.h"
#include "CurveEditorCommands.h"

class SVoxelCurveToolTip : public SToolTip
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ATTRIBUTE(bool, IsEnabled)
		SLATE_ATTRIBUTE(FText, InputText)
		SLATE_ATTRIBUTE(FText, OutputText)
	};

	void Construct(const FArguments& InArgs)
	{
		IsEnabled = InArgs._IsEnabled;

		SToolTip::Construct(
			SToolTip::FArguments()
			.BorderImage(FCoreStyle::Get().GetBrush("ToolTip.BrightBackground"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Text(InArgs._InputText)
					.Font(FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont"))
					.ColorAndOpacity(FLinearColor::Black)
				]
				+ SVerticalBox::Slot()
				[
					SNew(STextBlock)
					.Text(InArgs._OutputText)
					.Font(FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont"))
					.ColorAndOpacity(FLinearColor::Black)
				]
			]);
	}

	//~ Begin SToolTip Interface
	virtual bool IsEmpty() const override { return !IsEnabled.Get(); }
	//~ End SToolTip Interface

private:
	TAttribute<bool> IsEnabled;
};

void SVoxelCurvePanel::Construct(const FArguments& InArgs)
{
	CurveModel = InArgs._CurveModel;
	CommandList = InArgs._CommandList;

	CurveTint = InArgs._CurveTint;
	KeysTint = InArgs._KeysTint;
	TangentsTint = InArgs._TangentsTint;

	MaxHeight = InArgs._MaxHeight;

	Invalidate = InArgs._Invalidate;

	BindCommands();

	SetToolTip(
		SNew(SVoxelCurveToolTip)
		.IsEnabled_Lambda(MakeWeakPtrLambda(this, [this]
		{
			return CachedToolTip.IsSet();
		}))
		.InputText_Lambda(MakeWeakPtrLambda(this, [this]() -> FText
		{
			if (!CachedToolTip.IsSet())
			{
				return {};
			}

			return CachedToolTip->InputText;
		}))
		.OutputText_Lambda(MakeWeakPtrLambda(this, [this]() -> FText
		{
			if (!CachedToolTip.IsSet())
			{
				return {};
			}

			return CachedToolTip->OutputText;
		})));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 SVoxelCurvePanel::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	const ESlateDrawEffect DrawEffects = ShouldBeEnabled(bParentEnabled) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	const FSlateBrush* Background = FAppStyle::GetBrush(STATIC_FNAME("Brushes.Panel"));
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		Background,
		DrawEffects,
		Background->GetTint(InWidgetStyle));

	ConstCast(CurveModel)->PrepareData(AllottedGeometry, *this);
	DrawGridLines(AllottedGeometry, OutDrawElements, LayerId, DrawEffects);
	DrawCurve(AllottedGeometry, OutDrawElements, LayerId, DrawEffects);

	if (DragOperation)
	{
		DragOperation->OnPaint(AllottedGeometry, OutDrawElements, LayerId);
	}

	return LayerId + 60;
}

void SVoxelCurvePanel::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	SLeafWidget::OnMouseLeave(MouseEvent);

	HoveredKeyHandle.Reset();
	HoveredInput.Reset();

	if (DragOperation &&
		DragOperation->CancelOnMouseLeave())
	{
		DragOperation->OnCancel(MouseEvent);
		DragOperation = nullptr;
	}
}

FReply SVoxelCurvePanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (ActiveContextMenu.IsValid())
	{
		return FReply::Unhandled();
	}

	const FVector2D MousePixel = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	const bool bWasHovered = HoveredKeyHandle.IsSet() || HoveredInput.IsSet();
	FindClosestPoint(MousePixel);
	const bool bHovered = HoveredKeyHandle.IsSet() || HoveredInput.IsSet();

	if (!DragOperation)
	{
		if (bWasHovered != bHovered)
		{
			Invalidate.ExecuteIfBound();
		}

		return FReply::Unhandled();
	}

	Invalidate.ExecuteIfBound();
	return DragOperation->OnMouseMove(MousePixel, MouseEvent);
}

FReply SVoxelCurvePanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D MousePixel = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

	CachedMousePosition = MousePixel;

	if (FindClosestPoint(MousePixel) &&
		HoveredKeyHandle.IsSet())
	{
		TSet<FKeyHandle> SelectedKeys = CurveModel->SelectedKeys;
		if (MouseEvent.IsControlDown())
		{
			if (SelectedKeys.Contains(HoveredKeyHandle->Handle))
			{
				SelectedKeys.Remove(HoveredKeyHandle->Handle);
			}
			else
			{
				SelectedKeys.Add(HoveredKeyHandle->Handle);
			}
		}
		else if (!SelectedKeys.Contains(HoveredKeyHandle->Handle))
		{
			SelectedKeys = { HoveredKeyHandle->Handle };
		}

		CurveModel->SelectKeys(SelectedKeys);

		CurveModel->SelectedTangentType = HoveredKeyHandle->PointType;

		if (HoveredKeyHandle->PointType != FVoxelCurveKeyPoint::EKeyPointType::None)
		{
			DragOperation = MakeShared<FVoxelCurveDragOperation_MoveTangents>(SharedThis(this), CurveModel, MousePixel, MouseEvent.GetEffectingButton(), *HoveredKeyHandle);
			return SLeafWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
		}

		DragOperation = MakeShared<FVoxelCurveDragOperation_MoveKeys>(SharedThis(this), CurveModel, MousePixel, MouseEvent.GetEffectingButton());
		return SLeafWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
	}

	CurveModel->SelectedTangentType = FVoxelCurveKeyPoint::EKeyPointType::None;
	if (!MouseEvent.IsControlDown())
	{
		CurveModel->SelectKeys({});
	}

	DragOperation = MakeShared<FVoxelCurveDragOperation_Marquee>(SharedThis(this), CurveModel, MousePixel, MouseEvent.GetEffectingButton());
	return FReply::Handled();
}

FReply SVoxelCurvePanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D MousePixel = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (DragOperation)
	{
		if (DragOperation->IsDragging())
		{
			DragOperation->OnMouseUp(MousePixel, MouseEvent);
			DragOperation = nullptr;
			return FReply::Handled().ReleaseMouseCapture();
		}
		else
		{
			DragOperation->OnCancel(MouseEvent);
		}
		DragOperation = nullptr;
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (CurveModel->CachedCurveScreenBox.ContainsPoint(MousePixel))
		{
			CreateContextMenu(MyGeometry, MouseEvent);
			return FReply::Handled();
		}
	}

	return FReply::Handled();
}

FReply SVoxelCurvePanel::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (HoveredInput.IsSet() &&
		!HoveredKeyHandle.IsSet())
	{
		CurveModel->AddKeyAtHoveredPosition(MakeWeakPtr(this));
		return FReply::Handled();
	}

	return SLeafWidget::OnMouseButtonDoubleClick(InMyGeometry, InMouseEvent);
}

FReply SVoxelCurvePanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (CommandList->ProcessCommandBindings(InKeyEvent))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FCursorReply SVoxelCurvePanel::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	if (DragOperation &&
		DragOperation->IsDragging())
	{
		return FCursorReply::Cursor(EMouseCursor::Crosshairs);
	}

	return SLeafWidget::OnCursorQuery(MyGeometry, CursorEvent);
}

FVector2D SVoxelCurvePanel::ComputeDesiredSize(float X) const
{
	return {300.f, 300.f};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelCurvePanel::BindCommands()
{
	// Axis Snapping
	{
		CommandList->MapAction(
			FCurveEditorCommands::Get().SetAxisSnappingNone,
			FExecuteAction::CreateSP(this, &SVoxelCurvePanel::SetAxisSnapping, EAxisList::Type::None),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SVoxelCurvePanel::CompareAxisSnapping, EAxisList::Type::None));
		CommandList->MapAction(
			FCurveEditorCommands::Get().SetAxisSnappingHorizontal,
			FExecuteAction::CreateSP(this, &SVoxelCurvePanel::SetAxisSnapping, EAxisList::Type::X),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SVoxelCurvePanel::CompareAxisSnapping, EAxisList::Type::X));
		CommandList->MapAction(
			FCurveEditorCommands::Get().SetAxisSnappingVertical,
			FExecuteAction::CreateSP(this, &SVoxelCurvePanel::SetAxisSnapping, EAxisList::Type::Y),
			FCanExecuteAction(),
			FIsActionChecked::CreateSP(this, &SVoxelCurvePanel::CompareAxisSnapping, EAxisList::Type::Y));
	}

	CommandList->MapAction(
		FCurveEditorCommands::Get().AddKeyHovered,
		FExecuteAction::CreateSP(CurveModel.Get(), &FVoxelCurveModel::AddKeyAtHoveredPosition, MakeWeakPtr(this)));
	CommandList->MapAction(
		FCurveEditorCommands::Get().AddKeyToAllCurves,
		FExecuteAction::CreateSP(CurveModel.Get(), &FVoxelCurveModel::AddKeyAtHoveredPosition, MakeWeakPtr(this)));

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelCurvePanel::SetGridSpacing(const TOptional<float> NewGridSpacing, const bool bInput)
{
	if (bInput)
	{
		InputGridSpacing = NewGridSpacing;
	}
	else
	{
		OutputGridSpacing = NewGridSpacing;
	}
	Invalidate.ExecuteIfBound();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelCurvePanel::DrawGridLines(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	int32 BaseLayerId,
	ESlateDrawEffect DrawEffects) const
{
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("ToolTip.LargerFont");

	constexpr FLinearColor MajorGridColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.f);
	constexpr FLinearColor MinorGridColor = FLinearColor(0.1f, 0.1f, 0.1f, 0.5f);
	const FLinearColor LabelColor = FLinearColor::White.CopyWithNewOpacity(0.65f);

	constexpr float LabelOffsetPixels = 4.f;
	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

	TArray<FVector2D> LinePoints;
	LinePoints.Add(FVector2D::ZeroVector);
	LinePoints.Add(FVector2D::ZeroVector);

	float LastLabelPos = -FLT_MAX;
	for (const FVoxelCurveGridSpacing::FSlice& Slice : CurveModel->HorizontalSpacing.Slices)
	{
		float Line = FMath::RoundToFloat(CurveModel->GetHorizontalPosition(Slice.Value));

		float LabelPosX = Line - (Slice.LabelSize.X + LabelOffsetPixels) * 0.5f;
		bool bIsMajor = Slice.bIsMajor && LastLabelPos < LabelPosX;

		LinePoints[0].X = Line;
		LinePoints[1].X = Line;

		LinePoints[0].Y = CurveModel->CachedCurveScreenBox.Top;
		LinePoints[1].Y = CurveModel->CachedCurveScreenBox.Bottom + (bIsMajor ? LabelOffsetPixels : 0.f);

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			BaseLayerId + 1,
			PaintGeometry,
			LinePoints,
			DrawEffects,
			bIsMajor ? MajorGridColor : MinorGridColor,
			false);

		if (!bIsMajor)
		{
			continue;
		}

		LastLabelPos = Line + (Slice.LabelSize.X + LabelOffsetPixels) * 0.5f;

		LinePoints[0].X -= Slice.LabelSize.X * 0.5f;
		LinePoints[0].Y = LinePoints[1].Y + LabelOffsetPixels;

		const FPaintGeometry LabelGeometry = AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(LinePoints[0]));

		FSlateDrawElement::MakeText(
			OutDrawElements,
			BaseLayerId + 3,
			LabelGeometry,
			Slice.Label,
			FontInfo,
			DrawEffects,
			LabelColor);
	}

	LastLabelPos = FLT_MAX;
	for (const FVoxelCurveGridSpacing::FSlice& Slice : CurveModel->VerticalSpacing.Slices)
	{
		float Line = FMath::RoundToFloat(CurveModel->GetVerticalPosition(Slice.Value));

		float LabelPosY = Line + (Slice.LabelSize.Y + LabelOffsetPixels) * 0.5f;
		bool bIsMajor = Slice.bIsMajor && LastLabelPos > LabelPosY;

		LinePoints[0].Y = Line;
		LinePoints[1].Y = Line;
		
		LinePoints[0].X = CurveModel->CachedCurveScreenBox.Left - (bIsMajor ? LabelOffsetPixels : 0.f);
		LinePoints[1].X = CurveModel->CachedCurveScreenBox.Right;

		FSlateDrawElement::MakeLines(
			OutDrawElements,
			BaseLayerId + 1,
			PaintGeometry,
			LinePoints,
			DrawEffects,
			bIsMajor ? MajorGridColor : MinorGridColor,
			false);

		if (!bIsMajor)
		{
			continue;
		}

		LastLabelPos = Line - (Slice.LabelSize.Y + LabelOffsetPixels) * 0.5f;

		LinePoints[0].X = LabelOffsetPixels + (CurveModel->VerticalSpacing.MaxLabelSize.X - Slice.LabelSize.X);
		LinePoints[0].Y -= Slice.LabelSize.Y * 0.5f;

		const FPaintGeometry LabelGeometry = AllottedGeometry.ToPaintGeometry(FSlateLayoutTransform(LinePoints[0]));

		FSlateDrawElement::MakeText(
			OutDrawElements,
			BaseLayerId + 3,
			LabelGeometry,
			Slice.Label,
			FontInfo,
			DrawEffects,
			LabelColor);
	}
}

void SVoxelCurvePanel::DrawCurve(
	const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements,
	const int32 BaseLayerId,
	const ESlateDrawEffect DrawEffects) const
{
	auto IsInHeightRange = [&](const FVector2D& Point)
	{
		return
			CurveModel->CachedCurveScreenBox.Top - 0.5f <= Point.Y &&
			CurveModel->CachedCurveScreenBox.Bottom + 0.5f >= Point.Y;
	};

	auto InterpolateX = [](const FVector2D& From, const FVector2D& To, const float Value)
	{
		const float Alpha = (Value - From.Y) / FMath::Abs(From.Y - To.Y);
		return FMath::Lerp(From, To, Alpha);
	};

	bool bCurveIsHovered = HoveredKeyHandle.IsSet() || HoveredInput.IsSet();
	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();

	TArray<TArray<FVector2D>> Lines;
	Lines.Add({});

	TArray<FVector2D>* CurrentLine = &Lines[0];
	for (int32 Index = 1; Index < CurveModel->InterpolatedCurvePoints.Num() - 1; Index++)
	{
		const FVector2D& CurScreenPosition = CurveModel->InterpolatedCurvePoints[Index];

		if (!IsInHeightRange(CurScreenPosition))
		{
			continue;
		}

		const FVector2D& PrevScreenPosition = CurveModel->InterpolatedCurvePoints[Index - 1];
		if (!IsInHeightRange(PrevScreenPosition))
		{
			if (PrevScreenPosition.Y < CurveModel->CachedCurveScreenBox.Top)
			{
				CurrentLine->Add(InterpolateX(PrevScreenPosition, CurScreenPosition, CurveModel->CachedCurveScreenBox.Top));
			}
			else
			{
				CurrentLine->Add(InterpolateX(CurScreenPosition, PrevScreenPosition, CurveModel->CachedCurveScreenBox.Bottom));
			}
		}
		else if (Index == 1)
		{
			CurrentLine->Add(PrevScreenPosition);
		}

		CurrentLine->Add(CurScreenPosition);

		const FVector2D& NextScreenPosition = CurveModel->InterpolatedCurvePoints[Index + 1];
		if (!IsInHeightRange(NextScreenPosition))
		{
			if (NextScreenPosition.Y < CurveModel->CachedCurveScreenBox.Top)
			{
				CurrentLine->Add(InterpolateX(NextScreenPosition, CurScreenPosition, CurveModel->CachedCurveScreenBox.Top));
			}
			else
			{
				CurrentLine->Add(InterpolateX(CurScreenPosition, NextScreenPosition, CurveModel->CachedCurveScreenBox.Bottom));
			}

			CurrentLine = &Lines.Add_GetRef({});
		}
		else if (Index == CurveModel->InterpolatedCurvePoints.Num() - 2)
		{
			CurrentLine->Add(NextScreenPosition);
		}
	}

	for (const auto& LinePoints : Lines)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			BaseLayerId + 10,
			PaintGeometry,
			LinePoints,
			DrawEffects,
			CurveTint,
			true,
			bCurveIsHovered ? 3.5f : 2.f);
	}

	for (int32 Index = 0; Index < CurveModel->CachedKeyPoints.Num(); Index++)
	{
		const FVoxelCurveKeyPoint& Point = CurveModel->CachedKeyPoints[Index];

		FLinearColor PointTint = Point.PointType == FVoxelCurveKeyPoint::EKeyPointType::None ? KeysTint : TangentsTint;
		const bool bSelected = Point.PointType == FVoxelCurveKeyPoint::EKeyPointType::None ? CurveModel->SelectedKeys.Contains(Point.Handle) : Point.PointType == CurveModel->SelectedTangentType;
		if (!bSelected)
		{
			FLinearColor HSV = PointTint.LinearRGBToHSV();
			HSV.G = FMath::Clamp(HSV.G * 1.1f, 0.f, 255.f);
			HSV.B = FMath::Clamp(HSV.B * 2.f, 0.f, 255.f);
			PointTint = HSV.HSVToLinearRGB();
		}
		else
		{
			PointTint = FLinearColor::White;
		}

		FPaintGeometry PointGeometry = AllottedGeometry.ToPaintGeometry(
			Point.BrushSize,
			FSlateLayoutTransform(Point.ScreenPosition - (Point.BrushSize * 0.5f))
		);

		if (Point.LineDelta.X != 0.f ||
			Point.LineDelta.Y != 0.f)
		{
			TArray<FVector2D> LinePoints{ FVector2D::ZeroVector, FVector2D::ZeroVector };
			LinePoints[0] = Point.ScreenPosition + Point.LineDelta.GetSafeNormal() * (Point.BrushSize * 0.5f);
			LinePoints[1] = Point.ScreenPosition + Point.LineDelta;

			// Draw the connecting line - connecting lines are always drawn below everything else
			FSlateDrawElement::MakeLines(OutDrawElements, BaseLayerId + 20 - 1, PaintGeometry, LinePoints, DrawEffects, PointTint, true);
		}

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			BaseLayerId + 20 + Point.LayerBias,
			PointGeometry,
			Point.Brush,
			DrawEffects,
			PointTint);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool SVoxelCurvePanel::FindClosestPoint(const FVector2D& MousePixel)
{
	if (!ensure(CurveModel) ||
		!ensure(CurveModel->Curve))
	{
		return false;
	}

	CachedToolTip.Reset();
	HoveredKeyHandle.Reset();
	HoveredInput.Reset();

	const float Width = CurveModel->CachedCurveScreenBox.Right - CurveModel->CachedCurveScreenBox.Left;
	const float Height = CurveModel->CachedCurveScreenBox.Bottom - CurveModel->CachedCurveScreenBox.Top;

	float ClosestDistance = FLT_MAX;
	for (const FVoxelCurveKeyPoint& KeyPoint : CurveModel->CachedKeyPoints)
	{
		FVector2D HitTestSize = KeyPoint.BrushSize + 4.f;

		FSlateRect KeyRect = FSlateRect::FromPointAndExtent(KeyPoint.ScreenPosition - (HitTestSize / 2.f), HitTestSize);

		if (KeyRect.ContainsPoint(MousePixel))
		{
			const float DistanceSquared = (KeyRect.GetCenter() - MousePixel).SizeSquared();
			if (DistanceSquared < ClosestDistance)
			{
				ClosestDistance = DistanceSquared;
				HoveredKeyHandle = KeyPoint;
			}
		}
	}

	if (HoveredKeyHandle.IsSet())
	{
		const FRichCurveKey& Key = CurveModel->Curve->GetKey(HoveredKeyHandle->Handle);
		CachedToolTip = FVoxelCachedToolTipData(Key.Time, Key.Value);
		return true;
	}

	if (!CurveModel->CachedCurveScreenBox.ExtendBy(1.f).ContainsPoint(MousePixel))
	{
		return false;
	}

	constexpr float HoverProximityThresholdPx = 5.f;

	const float MinMousePos = ((MousePixel.X - HoverProximityThresholdPx) - CurveModel->CachedCurveScreenBox.Left) / Width;
	const double MinMouseInput = FMath::Lerp(CurveModel->CurveMin.X, CurveModel->CurveMax.X, MinMousePos);
	const float MaxMousePos = ((MousePixel.X + HoverProximityThresholdPx) - CurveModel->CachedCurveScreenBox.Left) / Width;
	const double MaxMouseInput = FMath::Lerp(CurveModel->CurveMin.X, CurveModel->CurveMax.X, MaxMousePos);

	const double MouseValA = CurveModel->CachedCurveScreenBox.Bottom - ((CurveModel->Curve->Eval(MinMouseInput) - CurveModel->CurveMin.Y)) * (Height / (CurveModel->CurveMax.Y - CurveModel->CurveMin.Y));
	const double MouseValB = CurveModel->CachedCurveScreenBox.Bottom - ((CurveModel->Curve->Eval(MaxMouseInput) - CurveModel->CurveMin.Y)) * (Height / (CurveModel->CurveMax.Y - CurveModel->CurveMin.Y));
	const FVector2D MinPos(MousePixel.X - HoverProximityThresholdPx, MouseValA);
	const FVector2D MaxPos(MousePixel.X + HoverProximityThresholdPx, MouseValB);

	const float Distance = (FMath::ClosestPointOnSegment2D(MousePixel, MinPos, MaxPos) - MousePixel).Size();
	if (Distance > HoverProximityThresholdPx)
	{
		return false;
	}

	HoveredInput = (MousePixel.X - CurveModel->CachedCurveScreenBox.Left) / Width;
	CachedToolTip = FVoxelCachedToolTipData(*HoveredInput, CurveModel->Curve->Eval(*HoveredInput));

	return true;
}

void SVoxelCurvePanel::CreateContextMenu(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	FMenuBuilder MenuBuilder(true, CommandList);
	if (HoveredKeyHandle.IsSet())
	{
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Cut);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Copy);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Paste);
		MenuBuilder.AddMenuEntry(FGenericCommands::Get().Delete);

		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().FlattenTangents);
		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().StraightenTangents);

		MenuBuilder.AddMenuSeparator();

		UE_503_ONLY(MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().InterpolationCubicSmartAuto));
		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().InterpolationCubicAuto);
		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().InterpolationCubicUser);
		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().InterpolationCubicBreak);
		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().InterpolationLinear);
		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().InterpolationConstant);
	}
	else
	{
		if (HoveredInput.IsSet())
		{
			MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().AddKeyHovered);
		}
		else
		{
			MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().AddKeyToAllCurves);
		}

		MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().PasteKeysHovered);
	}

	MenuBuilder.AddSeparator();

	MenuBuilder.AddMenuEntry(FCurveEditorCommands::Get().SelectAllKeys);

	const FWidgetPath WidgetPath = MouseEvent.GetEventPath() ? *MouseEvent.GetEventPath() : FWidgetPath();
	ActiveContextMenu = FSlateApplication::Get().PushMenu(AsShared(), WidgetPath, MenuBuilder.MakeWidget(), FSlateApplication::Get().GetCursorPos(), FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
}