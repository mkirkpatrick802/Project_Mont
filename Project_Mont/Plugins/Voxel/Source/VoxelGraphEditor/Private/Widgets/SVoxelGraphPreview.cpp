// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphPreview.h"
#include "VoxelActor.h"
#include "VoxelGraph.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelTerminalGraphRuntime.h"
#include "VoxelTerminalGraphInstance.h"
#include "VoxelParameterOverridesRuntime.h"
#include "Preview/VoxelNode_Preview.h"
#include "Preview/VoxelPreviewHandler.h"
#include "Widgets/SVoxelGraphPreviewImage.h"
#include "Widgets/SVoxelGraphPreviewStats.h"
#include "Widgets/SVoxelGraphPreviewRuler.h"
#include "Widgets/SVoxelGraphPreviewScale.h"
#include "Widgets/SVoxelGraphPreviewDepthSlider.h"

void SVoxelGraphPreview::Construct(const FArguments& Args)
{
	VOXEL_FUNCTION_COUNTER();

	SceneComponent = NewObject<UVoxelActorRootComponent>();
	TransformRef = FVoxelTransformRef::Make(*SceneComponent);

	FSlimHorizontalToolBarBuilder LeftToolbarBuilder(nullptr, FMultiBoxCustomization::None);
	LeftToolbarBuilder.SetStyle(&FAppStyle::Get(), "EditorViewportToolBar");
	LeftToolbarBuilder.SetLabelVisibility(EVisibility::Collapsed);

	LeftToolbarBuilder.BeginSection("Preview");
	{
		LeftToolbarBuilder.BeginBlockGroup();

		const auto AddAxisButton = [&](EVoxelAxis Axis)
		{
			FSlateIcon Icon;
			switch (Axis)
			{
			default: ensure(false);
			case EVoxelAxis::X: Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "StaticMeshEditor.ToggleShowTangents"); break;
			case EVoxelAxis::Y: Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "StaticMeshEditor.SetShowBinormals"); break;
			case EVoxelAxis::Z: Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "StaticMeshEditor.SetShowNormals"); break;
			}
			LeftToolbarBuilder.AddToolBarButton(FUIAction(
				MakeLambdaDelegate([this, Axis]
				{
					UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
					if (!TerminalGraph)
					{
						return;
					}

					// No transaction for preview
					// const FVoxelTransaction Transaction(Graph, "Set preview axis");
					TerminalGraph->PreviewConfig.Axis = Axis;

					if (DepthSlider)
					{
						DepthSlider->ResetValue(TerminalGraph->PreviewConfig.GetAxisLocation(), false);
					}
				}),
				MakeLambdaDelegate([]
				{
					return true;
				}),
				MakeLambdaDelegate([this, Axis]
				{
					const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
					if (!TerminalGraph)
					{
						return false;
					}

					return TerminalGraph->PreviewConfig.Axis == Axis;
				})),
				{},
				UEnum::GetDisplayValueAsText(Axis),
				UEnum::GetDisplayValueAsText(Axis),
				Icon,
				EUserInterfaceActionType::ToggleButton);
		};

		AddAxisButton(EVoxelAxis::X);
		AddAxisButton(EVoxelAxis::Y);
		AddAxisButton(EVoxelAxis::Z);

		LeftToolbarBuilder.EndBlockGroup();
	}

	LeftToolbarBuilder.AddSeparator();

	LeftToolbarBuilder.AddToolBarButton(FUIAction(
		MakeLambdaDelegate([this]
		{
			UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
			if (!TerminalGraph)
			{
				return;
			}

			// No transaction for preview
			// const FVoxelTransaction Transaction(Graph, "Reset view");

			TerminalGraph->PreviewConfig.Position = FVector::ZeroVector;
			TerminalGraph->PreviewConfig.Zoom = 1.f;

			if (DepthSlider)
			{
				DepthSlider->ResetValue(TerminalGraph->PreviewConfig.GetAxisLocation(), false);
			}
		}),
		MakeLambdaDelegate([this]
		{
			const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
			if (!TerminalGraph)
			{
				return false;
			}

			return
				!TerminalGraph->PreviewConfig.Position.IsZero() ||
				TerminalGraph->PreviewConfig.Zoom != 1.f;
		})),
		{},
		INVTEXT("Reset view"),
		INVTEXT("Reset view"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GenericCommands.Undo"),
		EUserInterfaceActionType::Button);

	LeftToolbarBuilder.EndSection();

	const TSharedRef<SWidget> ResolutionSelector =
		SNew(SComboButton)
		.ButtonStyle(FAppStyle::Get(), "EditorViewportToolBar.Button")
		.ComboButtonStyle(FAppStyle::Get(), "ViewportPinnedCommandList.ComboButton")
		.ContentPadding(FMargin(4.f, 0.f))
		.ToolTipText(INVTEXT("Texture resolution for preview. The higher the resolution, the longer it will take to compute preview"))
		.OnGetMenuContent_Lambda([this]
		{
			FMenuBuilder MenuBuilder(true, nullptr);

			MenuBuilder.BeginSection({}, INVTEXT("Resolutions"));

			static const TArray<int32> PreviewSizes =
			{
				256,
				512,
				1024,
				2048,
				4096
			};

			for (const int32 NewPreviewSize : PreviewSizes)
			{
				const FString Text = FString::Printf(TEXT("%dx%d"), NewPreviewSize, NewPreviewSize);

				MenuBuilder.AddMenuEntry(
					FText::FromString(Text),
					FText::FromString(Text),
					FSlateIcon(),
					FUIAction(
						MakeLambdaDelegate([=]
						{
							UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
							if (!TerminalGraph)
							{
								return;
							}

							const FVoxelTransaction Transaction(TerminalGraph, "Change preview size");
							TerminalGraph->PreviewConfig.Resolution = NewPreviewSize;

							QueueUpdate();
						}),
						MakeLambdaDelegate([]
						{
							return true;
						}),
						MakeLambdaDelegate([this, NewPreviewSize]
						{
							const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
							if (!TerminalGraph)
							{
								return false;
							}

							return TerminalGraph->PreviewConfig.Resolution == NewPreviewSize;
						})
					),
					{},
					EUserInterfaceActionType::Check);
			}

			MenuBuilder.EndSection();

			return MenuBuilder.MakeWidget();
		})
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			.Clipping(EWidgetClipping::ClipToBounds)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("CurveEditorTools.ActivateTransformTool"))
				.DesiredSizeOverride(FVector2D(16.f, 16.f))
			]
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.Padding(4.f, 0.f)
			.AutoWidth()
			[
				SNew(SVoxelDetailText)
				.Text_Lambda([this]
				{
					const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
					if (!TerminalGraph)
					{
						return INVTEXT("512x512");
					}

					return FText::FromString(FString::Printf(TEXT("%dx%d"), TerminalGraph->PreviewConfig.Resolution, TerminalGraph->PreviewConfig.Resolution));
				})
				.Clipping(EWidgetClipping::ClipToBounds)
			]
		];

	TSharedPtr<SImage> Image;
	TSharedPtr<SScaleBox> PreviewScaleBox;
	TSharedPtr<SVoxelGraphPreviewScale> PreviewScale;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(PreviewScaleBox, SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SAssignNew(PreviewImage, SVoxelGraphPreviewImage)
						.Width_Lambda([this]() -> float
						{
							const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
							if (!TerminalGraph)
							{
								return 512;
							}

							return TerminalGraph->PreviewConfig.Resolution;
						})
						.Height_Lambda([this]() -> float
						{
							const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
							if (!TerminalGraph)
							{
								return 512;
							}

							return TerminalGraph->PreviewConfig.Resolution;
						})
						.Cursor(EMouseCursor::Crosshairs)
						[
							SAssignNew(Image, SImage)
							.Image_Lambda([this]
							{
								return PreviewHandler ? PreviewHandler->GetBrush_Texture() : nullptr;
							})
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Bottom)
					[
						SNew(SBox)
						.Visibility(EVisibility::HitTestInvisible)
						[
							SAssignNew(PreviewScale, SVoxelGraphPreviewScale)
							.Resolution_Lambda([this]
							{
								const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
								if (!TerminalGraph)
								{
									return 512;
								}

								return TerminalGraph->PreviewConfig.Resolution;
							})
							.Value_Lambda([this]
							{
								return GetPixelToWorld().GetScaleVector().X;
							})
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SBox)
						.Visibility(EVisibility::HitTestInvisible)
						[
							SAssignNew(PreviewRuler, SVoxelGraphPreviewRuler)
							.Resolution_Lambda([this]
							{
								const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
								if (!TerminalGraph)
								{
									return 512;
								}

								return TerminalGraph->PreviewConfig.Resolution;
							})
							.Value_Lambda([this]
							{
								return GetPixelToWorld().GetScaleVector().X;
							})
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(.0f, .0f, .0f, .3f))
						.Visibility_Lambda([this]
						{
							return
								PreviewHandler &&
								PreviewHandler->IsProcessing() &&
								ProcessingStartTime != 0 &&
								FPlatformTime::Seconds() - ProcessingStartTime > 1.f
								? EVisibility::Visible
								: EVisibility::Hidden;
							})
						[
							SNew(SScaleBox)
							.IgnoreInheritedScale(true)
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SCircularThrobber)
							]
						]
					]
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SBox)
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Center)
				.Visibility(EVisibility::HitTestInvisible)
				.Padding(FMargin(20.f, 0.f))
				[
					SNew(STextBlock)
					.Font(FVoxelEditorUtilities::Font())
					.Visibility_Lambda([this]
					{
						return Message.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
					})
					.Text_Lambda([this]
					{
						return FText::FromString(Message);
					})
					.ColorAndOpacity(FLinearColor::White)
					.ShadowOffset(FVector2D(1.f))
					.ShadowColorAndOpacity(FLinearColor::Black)
					.AutoWrapText(true)
					.Justification(ETextJustify::Center)
				]
			]
			+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(20.f, 0.f)
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						LeftToolbarBuilder.MakeWidget()
					]
					+ SHorizontalBox::Slot()
					.Padding(20.f, 0.f)
					.HAlign(HAlign_Right)
					[
						ResolutionSelector
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			.Padding(4.0f)
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Fill)
				.WidthOverride(150.f)
				[
					SAssignNew(DepthSliderContainer, SBox)
				]
			]
		]
	];

	PreviewRuler->WeakSizeWidget = PreviewScaleBox;
	PreviewScale->SizeWidget = PreviewScaleBox;

	PreviewStats = SNew(SVoxelGraphPreviewStats);

	Image->SetOnMouseMove(FPointerEventHandler::CreateLambda([this](const FGeometry&, const FPointerEvent) -> FReply
	{
		if (!bIsCoordinateLocked)
		{
			UpdateStats();
		}

		return FReply::Unhandled();
	}));

	TransformRef.AddOnChanged(MakeWeakPtrDelegate(this, [this](const FMatrix&)
	{
		UpdateStats();
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphPreview::SetTerminalGraph(UVoxelTerminalGraph* NewTerminalGraph)
{
	QueueUpdate();

	WeakTerminalGraph = NewTerminalGraph;

	if (!NewTerminalGraph)
	{
		OnEdGraphChangedPtr.Reset();
		DepthSlider.Reset();
		DepthSliderContainer->SetContent(SNullWidget::NullWidget);
		return;
	}

	OnEdGraphChangedPtr = MakeSharedVoid();

	GVoxelGraphTracker->OnEdGraphChanged(NewTerminalGraph->GetEdGraph()).Add(FOnVoxelGraphChanged::Make(OnEdGraphChangedPtr, this, [=]
	{
		QueueUpdate();
	}));

	DepthSlider =
		SNew(SVoxelGraphPreviewDepthSlider)
		.ToolTipText(INVTEXT("Depth along the axis being previewed"))
		.ValueText(INVTEXT("Depth"))
		.Value(NewTerminalGraph->PreviewConfig.GetAxisLocation())
		.MinValue(NewTerminalGraph->PreviewConfig.GetAxisLocation() - 100.f)
		.MaxValue(NewTerminalGraph->PreviewConfig.GetAxisLocation() + 100.f)
		.OnValueChanged_Lambda([=](const float NewValue)
		{
			UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
			if (!TerminalGraph)
			{
				return;
			}

			// No transaction for preview
			// const FVoxelTransaction Transaction(Graph, "Change depth");
			TerminalGraph->PreviewConfig.SetAxisLocation(NewValue);

			QueueUpdate();
		});

	DepthSliderContainer->SetContent(DepthSlider.ToSharedRef());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphPreview::GetPreviewStats() const
{
	return PreviewStats.ToSharedRef();
}

void SVoxelGraphPreview::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(SceneComponent);

	if (PreviewHandler)
	{
		PreviewHandler->AddStructReferencedObjects(Collector);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphPreview::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	VOXEL_FUNCTION_COUNTER();

	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (ensure(SceneComponent))
	{
		const FMatrix PixelToWorld = GetPixelToWorld();
		const FTransform PixelToWorldTransform(PixelToWorld);
		ensure(PixelToWorld.Equals(PixelToWorldTransform.ToMatrixWithScale()));

		if (!SceneComponent->GetComponentTransform().Equals(PixelToWorldTransform))
		{
			SceneComponent->SetWorldTransform(PixelToWorldTransform);
			FVoxelTransformRef::NotifyTransformChanged(*SceneComponent);
		}
	}

	if (bUpdateQueued)
	{
		bUpdateQueued = false;
		Update();
	}

	if (PreviewHandler &&
		PreviewHandler->IsProcessing())
	{
		if (ProcessingStartTime == 0)
		{
			ProcessingStartTime = FPlatformTime::Seconds();
		}
	}
	else
	{
		ProcessingStartTime = 0;
	}
}

FReply SVoxelGraphPreview::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		bLockCoordinatePending = true;
		return FReply::Handled();
	}

	const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);
	if (!bIsMiddleMouseButtonDown)
	{
		return FReply::Unhandled();
	}

	PreviewRuler->StartRuler(MouseEvent.GetScreenSpacePosition(), PreviewImage->GetCachedGeometry().AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()));

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

FReply SVoxelGraphPreview::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bLockCoordinatePending)
		{
			const FMatrix PixelToWorld = GetPixelToWorld();

			FVector2D MouseImagePosition = PreviewImage->GetCachedGeometry().AbsoluteToLocal(MousePosition);
			MouseImagePosition.Y = PreviewImage->GetCachedGeometry().Size.Y - MouseImagePosition.Y;

			if (bIsCoordinateLocked)
			{
				const FVector2D OldPixelPosition = FVector2D(PixelToWorld.InverseTransformPosition(LockedCoordinate_WorldSpace));
				if (FVector2D::Distance(OldPixelPosition, MouseImagePosition) < 20.f)
				{
					PreviewImage->ClearLockedPosition();

					bIsCoordinateLocked = false;
					return FReply::Handled();
				}
			}

			LockedCoordinate_WorldSpace = PixelToWorld.TransformPosition(FVector(MouseImagePosition.X, MouseImagePosition.Y, 0.f));
			bIsCoordinateLocked = true;

			PreviewImage->SetLockedPosition(LockedCoordinate_WorldSpace);

			if (!PreviewImage->UpdateLockedPosition(PixelToWorld, PreviewImage->GetCachedGeometry().Size))
			{
				bIsCoordinateLocked = false;
			}

			UpdateStats();
		}
		return FReply::Handled();
	}

	if (MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
	{
		return FReply::Unhandled();
	}

	PreviewRuler->StopRuler();

	return SCompoundWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
}

FReply SVoxelGraphPreview::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const bool bIsMouseButtonDown =
		MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) ||
		MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton);
	const bool bIsMiddleMouseButtonDown = MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton);

	MousePosition = MouseEvent.GetScreenSpacePosition();

	if (!bIsMouseButtonDown &&
		!bIsMiddleMouseButtonDown)
	{
		return FReply::Unhandled();
	}

	if (bIsMiddleMouseButtonDown)
	{
		if (!MouseEvent.GetCursorDelta().IsNearlyZero())
		{
			PreviewRuler->UpdateRuler(MousePosition, PreviewImage->GetCachedGeometry().AbsoluteToLocal(MousePosition));
		}
		return FReply::Handled();
	}

	const FVector2D PixelDelta = TransformVector(Inverse(PreviewImage->GetCachedGeometry().GetAccumulatedRenderTransform()), MouseEvent.GetCursorDelta());
	const FVector WorldDelta = GetPixelToWorld().TransformVector(FVector(-PixelDelta.X, PixelDelta.Y, 0));

	if (WorldDelta.IsNearlyZero())
	{
		return FReply::Handled();
	}

	bLockCoordinatePending = false;

	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!TerminalGraph)
	{
		return FReply::Handled();
	}

	TerminalGraph->PreviewConfig.Position += WorldDelta;

	if (DepthSlider)
	{
		DepthSlider->ResetValue(TerminalGraph->PreviewConfig.GetAxisLocation(), true);
	}

	QueueUpdate();

	return FReply::Handled();
}

FReply SVoxelGraphPreview::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const double Delta = MouseEvent.GetWheelDelta();
	if (FMath::IsNearlyZero(Delta))
	{
		return FReply::Handled();
	}

	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!TerminalGraph)
	{
		return FReply::Handled();
	}

	const double NewZoom = TerminalGraph->PreviewConfig.Zoom * (1. - FMath::Clamp(Delta, -0.5, 0.5));

	const FVector2D PixelDelta = TransformPoint(Inverse(PreviewImage->GetCachedGeometry().GetAccumulatedRenderTransform()), MouseEvent.GetScreenSpacePosition());
	const FVector OldPosition = GetPixelToWorld().TransformPosition(FVector(PixelDelta, 0));

	TerminalGraph->PreviewConfig.Zoom = FMath::Clamp(NewZoom, 1, 1.e8);

	const FVector NewPosition = GetPixelToWorld().TransformPosition(FVector(PixelDelta, 0));

	FVector PositionDelta = NewPosition - OldPosition;
	switch (TerminalGraph->PreviewConfig.Axis)
	{
	case EVoxelAxis::X: PositionDelta.Z *= -1.f; break;
	case EVoxelAxis::Y: PositionDelta.Z *= -1.f; break;
	case EVoxelAxis::Z: PositionDelta.Y *= -1.f; break;
	}

	TerminalGraph->PreviewConfig.Position -= PositionDelta;

	QueueUpdate();

	return FReply::Handled();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphPreview::Update()
{
	VOXEL_FUNCTION_COUNTER();

	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!TerminalGraph)
	{
		Message = {};
		return;
	}
	const FVoxelSerializedGraph& SerializedGraph = TerminalGraph->GetRuntime().GetSerializedGraph();

	if (!SerializedGraph.PreviewHandler.IsValid())
	{
		PreviewHandler.Reset();

		Message = "Press R to preview the selected node";
		return;
	}

	Message = {};

	const TSharedRef<FVoxelPreviewHandler> NewPreviewHandler = SerializedGraph.PreviewHandler.Get<FVoxelPreviewHandler>().MakeSharedCopy();

	if (PreviewHandler &&
		PreviewHandler->PreviewSize != TerminalGraph->PreviewConfig.Resolution)
	{
		PreviewHandler.Reset();
	}

	const FVoxelGraphNodeRef NodeRef
	{
		FVoxelTerminalGraphRef(TerminalGraph),
		FVoxelGraphConstants::NodeId_Preview,
		"Preview Node",
		{}
	};

	FVoxelGraphPinRef PinRef;
	PinRef.NodeRef = NodeRef;
	PinRef.PinName = VOXEL_PIN_NAME(FVoxelNode_Preview, ValuePin);

	if (PreviewHandler &&
		PreviewHandler->PinRef != PinRef)
	{
		PreviewHandler.Reset();
	}

	if (PreviewHandler &&
		!PreviewHandler->Equals_UPropertyOnly(*NewPreviewHandler))
	{
		PreviewHandler.Reset();
	}

	if (PreviewHandler)
	{
		return;
	}

	PreviewHandler = NewPreviewHandler;

	FVoxelRuntimeInfoBase RuntimeInfoBase = FVoxelRuntimeInfoBase::MakePreview();
	RuntimeInfoBase.LocalToWorld = TransformRef;
	RuntimeInfoBase.bParallelTasks = true;
	// Destroy handled by FVoxelPreviewHandler::~FVoxelPreviewHandler
	const TSharedRef<FVoxelRuntimeInfo> RuntimeInfo = RuntimeInfoBase.MakeRuntimeInfo_RequiresDestroy();

	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = FVoxelTerminalGraphInstance::Make(
		TerminalGraph->GetGraph(),
		FVoxelTerminalGraphRef(TerminalGraph),
		RuntimeInfo,
		FVoxelParameterOverridesRuntime::Create(&TerminalGraph->GetGraph()));

	PreviewHandler->PreviewSize = TerminalGraph->PreviewConfig.Resolution;
	PreviewHandler->PinRef = PinRef;
	PreviewHandler->RuntimeInfo = RuntimeInfo;
	PreviewHandler->TerminalGraphInstance = TerminalGraphInstance;
	PreviewHandler->Create(SerializedGraph.PreviewedPinType);

	PreviewStats->Rows.Reset();
	PreviewHandler->BuildStats([&](
		const FString& Name,
		const FString& Tooltip,
		const TFunction<FString()>& GetValue)
	{
		const TSharedRef<SVoxelGraphPreviewStats::FRow> StatsRow = MakeVoxelShared<SVoxelGraphPreviewStats::FRow>();
		StatsRow->Header = FText::FromString(Name);
		StatsRow->Tooltip = FText::FromString(Tooltip);
		StatsRow->Value = MakeAttributeLambda([=]
		{
			return FText::FromString(GetValue());
		});
		PreviewStats->Rows.Add(StatsRow);
	});

	PreviewStats->RowsView->RequestListRefresh();

	UpdateStats();
}

void SVoxelGraphPreview::UpdateStats()
{
	if (!PreviewHandler)
	{
		return;
	}

	const FMatrix PixelToWorld = GetPixelToWorld();

	if (!PreviewImage->UpdateLockedPosition(PixelToWorld, PreviewImage->GetCachedGeometry().Size))
	{
		bIsCoordinateLocked = false;
	}

	FVector2D LocalMousePosition;
	if (bIsCoordinateLocked)
	{
		LocalMousePosition = FVector2D(PixelToWorld.InverseTransformPosition(LockedCoordinate_WorldSpace));
	}
	else
	{
		LocalMousePosition = PreviewImage->GetCachedGeometry().AbsoluteToLocal(MousePosition);
		LocalMousePosition.Y = PreviewImage->GetCachedGeometry().Size.Y - LocalMousePosition.Y;
	}

	PreviewHandler->UpdateStats(LocalMousePosition);
}

FMatrix SVoxelGraphPreview::GetPixelToWorld() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!TerminalGraph)
	{
		return FMatrix::Identity;
	}

	const FMatrix Matrix = INLINE_LAMBDA -> FMatrix
	{
		const FVector X = FVector::UnitX();
		const FVector Y = FVector::UnitY();
		const FVector Z = FVector::UnitZ();

		switch (TerminalGraph->PreviewConfig.Axis)
		{
		default: ensure(false);
		case EVoxelAxis::X: return FMatrix(Y, Z, X, FVector::ZeroVector);
		case EVoxelAxis::Y: return FMatrix(X, Z, -Y, FVector::ZeroVector);
		case EVoxelAxis::Z: return FMatrix(X, Y, Z, FVector::ZeroVector);
		}
	};

	return
		FScaleMatrix(1. / TerminalGraph->PreviewConfig.Resolution) *
		FScaleMatrix(2.) *
		FTranslationMatrix(-FVector::OneVector) *
		FScaleMatrix(FVector(TerminalGraph->PreviewConfig.Zoom, TerminalGraph->PreviewConfig.Zoom, 1.f)) *
		FRotationMatrix(Matrix.Rotator()) *
		FTranslationMatrix(TerminalGraph->PreviewConfig.Position);
}