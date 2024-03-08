// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphNode.h"
#include "VoxelNode.h"
#include "SVoxelToolTip.h"
#include "SCommentBubble.h"
#include "VoxelGraphToolkit.h"
#include "SLevelOfDetailBranchNode.h"
#include "Nodes/VoxelGraphNode_Struct.h"

VOXEL_INITIALIZE_STYLE(GraphNodeEditor)
{
	Set("Node.Overlay.Warning", new IMAGE_BRUSH("Graphs/NodeOverlay_Warning", CoreStyleConstants::Icon32x32));
	Set("Node.Overlay.Debug", new CORE_IMAGE_BRUSH_SVG("Starship/Blueprints/Breakpoint_Valid", FVector2D(20.0f, 20.0f), FSlateColor(FColor::Cyan)));
	Set("Node.Overlay.Preview", new CORE_IMAGE_BRUSH_SVG("Starship/Blueprints/Breakpoint_Valid", FVector2D(20.0f, 20.0f), FSlateColor(FColor::Red)));

	Set("Pin.Buffer.Connected", new IMAGE_BRUSH("Graphs/BufferPin_Connected", FVector2D(15, 11)));
	Set("Pin.Buffer.Disconnected", new IMAGE_BRUSH("Graphs/BufferPin_Disconnected", FVector2D(15, 11)));
	Set("Pin.Buffer.Promotable.Inner", new IMAGE_BRUSH_SVG("Graphs/BufferPin_Promotable_Inner", CoreStyleConstants::Icon14x14));
	Set("Pin.Buffer.Promotable.Outer", new IMAGE_BRUSH_SVG("Graphs/BufferPin_Promotable_Outer", CoreStyleConstants::Icon14x14));

	Set("Pin.PointSet.Connected", new IMAGE_BRUSH("Graphs/PointSetPin_Connected", FVector2D(19, 15)));
	Set("Pin.PointSet.Disconnected", new IMAGE_BRUSH("Graphs/PointSetPin_Disconnected", FVector2D(19, 15)));

	Set("Pin.Surface.Connected", new IMAGE_BRUSH("Graphs/SurfacePin_Connected", FVector2D(19, 15)));
	Set("Pin.Surface.Disconnected", new IMAGE_BRUSH("Graphs/SurfacePin_Disconnected", FVector2D(19, 15)));

	Set("Node.Stats.TitleGloss", new BOX_BRUSH("Graphs/Node_Stats_Gloss", FMargin(12.0f / 64.0f)));
	Set("Node.Stats.ColorSpill", new BOX_BRUSH("Graphs/Node_Stats_Color_Spill", FMargin(8.0f / 64.0f, 3.0f / 32.0f, 0.f, 0.f)));

	Set("Icons.MinusCircle", new IMAGE_BRUSH_SVG("Graphs/Minus_Circle", CoreStyleConstants::Icon16x16));
}

void SVoxelGraphNode::Construct(const FArguments& Args, UVoxelGraphNode* InNode)
{
	VOXEL_FUNCTION_COUNTER();

	GraphNode = InNode;
	NodeDefinition = GetVoxelNode().GetNodeDefinition();

	SetCursor(EMouseCursor::CardinalCross);
	SetToolTip(
		SNew(SVoxelToolTip)
		.Text(GetNodeTooltip()));

	ContentScale.Bind(this, &SGraphNode::GetContentScale);

	UpdateGraphNode();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	LeftNodeBox.Reset();
	RightNodeBox.Reset();

	CategoryToChildPins.Reset();

	SetupErrorReporting();

	if (GetVoxelNode().IsCompact())
	{
		UpdateCompactNode();
	}
	else
	{
		UpdateStandardNode();
	}

	OverlayWidgets = {};

	FString Type;
	FString Tooltip;
	FString ColorType;
	if (GetVoxelNode().GetOverlayInfo(Type, Tooltip, ColorType))
	{
		const FSlateBrush* ImageBrush;
		if (Type == "Warning")
		{
			ImageBrush = FVoxelEditorStyle::Get().GetBrush(TEXT("Node.Overlay.Warning"));
		}
		else if (Type == "Lighting")
		{
			ImageBrush = FAppStyle::Get().GetBrush(TEXT("Graph.AnimationFastPathIndicator"));
		}
		else if (Type == "Clock")
		{
			ImageBrush = FAppStyle::Get().GetBrush(TEXT("Graph.Latent.LatentIcon"));
		}
		else if (Type == "Message")
		{
			ImageBrush = FAppStyle::Get().GetBrush(TEXT("Graph.Message.MessageIcon"));
		}
		else
		{
			ensure(false);
			return;
		}

		FLinearColor Color = FLinearColor::White;
		if (ColorType == "Red")
		{
			Color = FLinearColor::Red;
		}

		FOverlayWidget& OverlayWidget = OverlayWidgets.Add_GetRef({});
		OverlayWidget.Widget =
			SNew(SImage)
			.Image(ImageBrush)
			.ToolTipText(FText::FromString(Tooltip))
			.Visibility(EVisibility::Visible)
			.ColorAndOpacity(Color);

		OverlayWidget.BrushSize = ImageBrush->ImageSize;
	}
}

void SVoxelGraphNode::CreateOutputSideAddButton(const TSharedPtr<SVerticalBox> OutputBox)
{
	const TSharedRef<SWidget> Button = AddPinButtonContent(FText::FromString(NodeDefinition->GetAddPinLabel()), {});
	Button->SetCursor(EMouseCursor::Default);

	FMargin Padding = Settings->GetOutputPinPadding();
	Padding.Top += 6.0f;

	OutputBox->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Center)
		.Padding(Padding)
		[
			SNew(SBox)
			.Padding(0)
			.Visibility(this, &SVoxelGraphNode::IsAddPinButtonVisible)
			.ToolTipText(FText::FromString(NodeDefinition->GetAddPinTooltip()))
			[
				Button
			]
		];
}

EVisibility SVoxelGraphNode::IsAddPinButtonVisible() const
{
	return GetButtonVisibility(NodeDefinition->CanAddInputPin());
}

FReply SVoxelGraphNode::OnAddPin()
{
	const FVoxelTransaction Transaction(GetVoxelNode(), "Add input pin");
	NodeDefinition->AddInputPin();
	return FReply::Handled();
}

void SVoxelGraphNode::RequestRenameOnSpawn()
{
	if (GetVoxelNode().CanRenameOnSpawn())
	{
		RequestRename();
	}
}

TArray<FOverlayWidgetInfo> SVoxelGraphNode::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	float Offset = 0.f;

	for (const auto& OverlayWidget : OverlayWidgets)
	{
		if (!OverlayWidget.Widget)
		{
			continue;
		}

		FOverlayWidgetInfo Info;
		Info.OverlayOffset = OverlayWidget.GetLocation(WidgetSize, Offset);
		Info.Widget = OverlayWidget.Widget;

		Widgets.Add(Info);

		Offset += OverlayWidget.BrushSize.X + 5.f;
	}

	return Widgets;
}

void SVoxelGraphNode::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	if (GetVoxelNode().bEnableDebug)
	{
		const FSlateBrush* Brush = FVoxelEditorStyle::Get().GetBrush(TEXT("Node.Overlay.Debug"));

		FOverlayBrushInfo BrushInfo;
		BrushInfo.Brush = Brush;
		BrushInfo.OverlayOffset = -Brush->GetImageSize() / 2.f;
		Brushes.Add(BrushInfo);
	}

	if (GetVoxelNode().bEnablePreview)
	{
		const FSlateBrush* Brush = FVoxelEditorStyle::Get().GetBrush(TEXT("Node.Overlay.Preview"));

		FOverlayBrushInfo BrushInfo;
		BrushInfo.Brush = Brush;
		BrushInfo.OverlayOffset = -Brush->GetImageSize() / 2.f;
		Brushes.Add(BrushInfo);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphNode::UpdateStandardNode()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	const FSlateBrush* IconBrush;
	if (GraphNode &&
		GraphNode->ShowPaletteIconOnNode())
	{
		IconColor = FLinearColor::White;
		IconBrush = GraphNode->GetIconAndTint(IconColor).GetOptionalIcon();
	}
	else
	{
		IconColor = FLinearColor::White;
		IconBrush = nullptr;
	}

	TSharedPtr<SHorizontalBox> TitleBox;

	const TSharedRef<SWidget> TitleAreaWidget =
		SNew(SLevelOfDetailBranchNode)
		.UseLowDetailSlot(this, &SVoxelGraphNode::UseLowDetailNodeTitles)
		.LowDetail()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Graph.Node.ColorSpill"))
			.Padding(FMargin(75.0f, 22.0f)) // Saving enough space for a 'typical' title so the transition isn't quite so abrupt
			.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleColor)
		]
		.HighDetail()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Graph.Node.TitleGloss"))
				.ColorAndOpacity_Lambda(MakeWeakPtrLambda(this, [this]
				{
					return FSlateColor(FMath::Lerp(GetNodeTitleColor().GetSpecifiedColor(), FLinearColor::White, 0.2f));
				}))
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SAssignNew(TitleBox, SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("Graph.Node.ColorSpill"))
					// The extra margin on the right is for making the color spill stretch well past the node title
					.Padding(FMargin(10, 5, 30, 3))
					.BorderBackgroundColor(this, &SGraphNode::GetNodeTitleColor)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Top)
						.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
						.AutoWidth()
						[
							SNew(SImage)
							.Image(IconBrush)
							.ColorAndOpacity(this, &SGraphNode::GetNodeTitleIconColor)
						]
						+ SHorizontalBox::Slot()
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								CreateTitleWidget(NodeTitle)
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								NodeTitle
							]
						]
					]
				]
			]
			+ SOverlay::Slot()
			.VAlign(VAlign_Top)
			[
				SNew(SBorder)
				.Visibility(EVisibility::HitTestInvisible)
				.BorderImage(FAppStyle::GetBrush("Graph.Node.TitleHighlight"))
				.BorderBackgroundColor(FLinearColor::White)
				[
					SNew(SSpacer)
					.Size(FVector2D(20, 20))
				]
			]
		];

	if (TitleBox)
	{
		CreateStandardNodeTitleButtons(TitleBox);
	}

	TSharedPtr<SVerticalBox> InnerVerticalBox;
	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(Settings->GetNonPinNodeBodyPadding())
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Graph.Node.Body"))
				.ColorAndOpacity(this, &SVoxelGraphNode::GetNodeBodyColor)
			]
			+ SOverlay::Slot()
			[
				SAssignNew(InnerVerticalBox, SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				.Padding(Settings->GetNonPinNodeBodyPadding())
				[
					TitleAreaWidget
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				[
					CreateNodeContentArea()
				]
			]
		]
	];

	const TSharedRef<SCommentBubble> CommentBubble =
		SNew(SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.OnToggled(this, &SGraphNode::OnCommentBubbleToggled)
		.ColorAndOpacity(GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
	.SlotOffset(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetOffset))
	.SlotSize(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetSize))
	.AllowScaling(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
	.VAlign(VAlign_Top)
	[
		CommentBubble
	];

	if (NodeDefinition->OverridePinsOrder())
	{
		CreateCategorizedPinWidgets();
	}
	else
	{
		CreatePinWidgets();
	}

	CreateInputSideAddButton(LeftNodeBox);
	UpdateBottomContent(InnerVerticalBox);
	CreateAdvancedCategory(InnerVerticalBox);

	InnerVerticalBox->AddSlot()
	.AutoHeight()
	.Padding(Settings->GetNonPinNodeBodyPadding())
	[
		MakeStatWidget()
	];

	InnerVerticalBox->AddSlot()
	.AutoHeight()
	[
		ErrorReporting->AsWidget()
	];
}

void SVoxelGraphNode::UpdateCompactNode()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<SNodeTitle> NodeTitle = SNew(SNodeTitle, GraphNode);

	constexpr float MinNodePadding = 55.f;
	constexpr float MaxNodePadding = 180.0f;
	constexpr float PaddingIncrementSize = 20.0f;

	const float PinPaddingRight = FMath::Clamp(MinNodePadding + NodeTitle->GetHeadTitle().ToString().Len() * PaddingIncrementSize, MinNodePadding, MaxNodePadding);

	const TSharedRef<SOverlay> NodeOverlay =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, PinPaddingRight, 0.f)
		[
			// LEFT
			SAssignNew(LeftNodeBox, SVerticalBox)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(45.f, 0.f, 45.f, 0.f)
		[
			// MIDDLE
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Center)
			.AutoHeight()
			[
				SNew(STextBlock)
				.TextStyle(FAppStyle::Get(), "Graph.CompactNode.Title")
				.Text(&NodeTitle.Get(), &SNodeTitle::GetHeadTitle)
				.WrapTextAt(128.0f)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				NodeTitle
			]
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(55.f, 0.f, 0.f, 0.f)
		[
			// RIGHT
			SAssignNew(RightNodeBox, SVerticalBox)
		];

	this->GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Graph.VarNode.Body"))
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Graph.VarNode.Gloss"))
			]
			+ SOverlay::Slot()
			.Padding(FMargin(0.f, 3.f, 0.f, 0.f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.f, 0.f, 0.f, 4.f))
				[
					NodeOverlay
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					MakeStatWidget()
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			ErrorReporting->AsWidget()
		]
	];

	CreatePinWidgets();

	// Hide pin labels
	for (const TSharedRef<SGraphPin>& Pin : InputPins)
	{
		if (!Pin->GetPinObj()->ParentPin)
		{
			Pin->SetShowLabel(false);
		}
	}
	for (const TSharedRef<SGraphPin>& Pin : OutputPins)
	{
		if (!Pin->GetPinObj()->ParentPin)
		{
			Pin->SetShowLabel(false);
		}
	}

	const TSharedRef<SCommentBubble> CommentBubble =
		SNew(SCommentBubble)
		.GraphNode(GraphNode)
		.Text(this, &SGraphNode::GetNodeComment)
		.OnTextCommitted(this, &SGraphNode::OnCommentTextCommitted)
		.ColorAndOpacity(GetDefault<UGraphEditorSettings>()->DefaultCommentNodeTitleColor)
		.AllowPinning(true)
		.EnableTitleBarBubble(true)
		.EnableBubbleCtrls(true)
		.GraphLOD(this, &SGraphNode::GetCurrentLOD)
		.IsGraphNodeHovered(this, &SVoxelGraphNode::IsHovered);

	GetOrAddSlot(ENodeZone::TopCenter)
	.SlotOffset(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetOffset))
	.SlotSize(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::GetSize))
	.AllowScaling(MakeAttributeSP(&CommentBubble.Get(), &SCommentBubble::IsScalingAllowed))
	.VAlign(VAlign_Top)
	[
		CommentBubble
	];

	CreateInputSideAddButton(LeftNodeBox);
	CreateOutputSideAddButton(RightNodeBox);
}

///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphNode::CreateCategorizedPinWidgets()
{
	TArray<UEdGraphPin*> Pins = GraphNode->Pins;

	TMap<FName, TArray<UEdGraphPin*>> ParentToSplitPins;
	for (UEdGraphPin* Pin : Pins)
	{
		if (!Pin->ParentPin)
		{
			continue;
		}

		ParentToSplitPins.FindOrAdd(Pin->ParentPin->PinName, {}).Add(Pin);
	}

	for (const auto& It : ParentToSplitPins)
	{
		for (UEdGraphPin* Pin : It.Value)
		{
			Pins.RemoveSwap(Pin);
		}
	}

	Inputs = NodeDefinition->GetInputs();
	Outputs = NodeDefinition->GetOutputs();

	if (Inputs)
	{
		for (const TSharedRef<IVoxelNodeDefinition::FNode>& Node : Inputs->GetChildren())
		{
			CreateCategoryPinWidgets(Node, Pins, ParentToSplitPins, LeftNodeBox, true);
		}
	}

	if (Outputs)
	{
		for (const TSharedRef<IVoxelNodeDefinition::FNode>& Node : Outputs->GetChildren())
		{
			CreateCategoryPinWidgets(Node, Pins, ParentToSplitPins, RightNodeBox, false);
		}
	}

	for (UEdGraphPin* Pin : Pins)
	{
		ensure(Pin->bOrphanedPin);
		AddStandardNodePin(
			Pin,
			nullptr,
			Pin->Direction == EGPD_Input ? LeftNodeBox : RightNodeBox);
	}
}

void SVoxelGraphNode::CreateCategoryPinWidgets(
	const TSharedRef<IVoxelNodeDefinition::FNode>& Node,
	TArray<UEdGraphPin*>& Pins,
	TMap<FName, TArray<UEdGraphPin*>>& ParentToSplitPins,
	const TSharedPtr<SVerticalBox>& TargetContainer,
	const bool bInput)
{
	if (Node->IsPin())
	{
		if (UEdGraphPin** PinPtr = Pins.FindByPredicate([&Node](const UEdGraphPin* Pin)
		{
			return Node->Name == Pin->PinName;
		}))
		{
			if (UEdGraphPin* Pin = *PinPtr)
			{
				AddStandardNodePin(Pin, Node, TargetContainer);
				Pins.RemoveSwap(Pin);
			}
		}
		if (TArray<UEdGraphPin*>* SplitPinsPtr = ParentToSplitPins.Find(Node->Name))
		{
			for (UEdGraphPin* SplitPin : *SplitPinsPtr)
			{
				AddStandardNodePin(SplitPin, Node, TargetContainer);
			}

			ParentToSplitPins.Remove(Node->Name);
		}
		return;
	}

	const TSharedPtr<SVerticalBox> PinsBox = CreateCategoryWidget(Node, bInput);

	if (TargetContainer != PinsBox)
	{
		TargetContainer->AddSlot()
		.Padding(bInput ? FMargin(Settings->GetInputPinPadding().Left, 0.f, 0.f, 0.f) : FMargin(0.f, 0.f, Settings->GetOutputPinPadding().Right, 0.f))
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		[
			PinsBox.ToSharedRef()
		];
	}

	for (const TSharedRef<IVoxelNodeDefinition::FNode>& InnerNode : Node->GetChildren())
	{
		CreateCategoryPinWidgets(InnerNode, Pins, ParentToSplitPins, PinsBox, bInput);
	}
}

void SVoxelGraphNode::AddStandardNodePin(
	UEdGraphPin* Pin,
	const TSharedPtr<IVoxelNodeDefinition::FNode>& Node,
	const TSharedPtr<SVerticalBox>& TargetContainer)
{
	ensure(!Node || Node->IsPin());

	if (!ensure(Pin))
	{
		return;
	}

	// ShouldPinBeShown
	if (!ShouldPinBeHidden(Pin))
	{
		return;
	}

	const TSharedPtr<SGraphPin> NewPin = CreatePinWidget(Pin);
	if (!ensure(NewPin.IsValid()))
	{
		return;
	}
	const TSharedRef<SGraphPin> PinWidget = NewPin.ToSharedRef();

	PinWidget->SetOwner(SharedThis(this));

	if (Node)
	{
		for (TSharedPtr<IVoxelNodeDefinition::FNode> Parent = Node->WeakParent.Pin(); Parent; Parent = Parent->WeakParent.Pin())
		{
			CategoryToChildPins.FindOrAdd(Parent->GetConcatenatedPath()).Add(PinWidget);
		}
	}

	PinWidget->SetVisibility(MakeAttributeLambda([=]
	{
		if (Pin->bHidden ||
			!NodeDefinition->IsPinVisible(Pin, GraphNode))
		{
			return EVisibility::Collapsed;
		}

		if (!Node)
		{
			return EVisibility::Visible;
		}

		const TSharedPtr<IVoxelNodeDefinition::FNode> Parent = Node->WeakParent.Pin();
		if (!Parent)
		{
			return EVisibility::Visible;
		}

		return
			GetVoxelNode().IsVisible(*Parent) ||
			Pin->LinkedTo.Num() > 0
			? EVisibility::Visible
			: EVisibility::Collapsed;
	}));

	TargetContainer->AddSlot()
		.AutoHeight()
		.HAlign(Pin->Direction == EGPD_Input ? HAlign_Left : HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(Pin->Direction == EGPD_Input ? Settings->GetInputPinPadding() : Settings->GetOutputPinPadding())
		[
			PinWidget
		];
	(Pin->Direction == EGPD_Input ? InputPins : OutputPins).Add(PinWidget);
}

TSharedRef<SVerticalBox> SVoxelGraphNode::CreateCategoryWidget(
	const TSharedRef<IVoxelNodeDefinition::FNode>& Node,
	const bool bIsInput)
{
	ensure(!Node->IsPin());

	if (Node->GetPath().Num() == 1 &&
		Node->GetPath()[0] == STATIC_FNAME("Advanced"))
	{
		bCreateAdvancedCategory = true;
		return bIsInput ? LeftNodeBox.ToSharedRef() : RightNodeBox.ToSharedRef();
	}

	TSharedPtr<SHorizontalBox> NameRow;
	TSharedPtr<SHorizontalBox> ButtonContent;
	TSharedPtr<SButton> Button;

	const TSharedRef<SVerticalBox> Result =
		SNew(SVerticalBox)
		.Visibility_Lambda([=]
		{
			const TSharedPtr<IVoxelNodeDefinition::FNode> Parent = Node->WeakParent.Pin();
			if (!Parent)
			{
				return EVisibility::Visible;
			}

			if (GetVoxelNode().IsVisible(*Parent))
			{
				return EVisibility::Visible;
			}

			const TArray<TWeakPtr<SGraphPin>>* PinsPtr = CategoryToChildPins.Find(Node->GetConcatenatedPath());
			if (!ensure(PinsPtr))
			{
				return EVisibility::Collapsed;
			}

			for (const TWeakPtr<SGraphPin>& WeakPinWidget : *PinsPtr)
			{
				const TSharedPtr<SGraphPin> PinWidget = WeakPinWidget.Pin();
				if (!PinWidget)
				{
					continue;
				}

				const UEdGraphPin* Pin = PinWidget->GetPinObj();
				if (!Pin)
				{
					continue;
				}

				if (Pin->LinkedTo.Num() > 0)
				{
					return EVisibility::Visible;
				}
			}

			return EVisibility::Collapsed;
		})
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(bIsInput ? HAlign_Left : HAlign_Right)
		[
			SAssignNew(NameRow, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SAssignNew(Button, SButton)
				.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
				.ContentPadding(0)
				.VAlign(VAlign_Center)
				.HAlign(bIsInput ? HAlign_Left : HAlign_Right)
				.ClickMethod(EButtonClickMethod::MouseDown)
				.Cursor(EMouseCursor::Default)
				.OnClicked_Lambda([=]
				{
					TSet<FName>& CollapsedCategories = bIsInput ? GetVoxelNode().CollapsedInputCategories : GetVoxelNode().CollapsedOutputCategories;
					if (CollapsedCategories.Contains(Node->GetConcatenatedPath()))
					{
						CollapsedCategories.Remove(Node->GetConcatenatedPath());
					}
					else
					{
						CollapsedCategories.Add(Node->GetConcatenatedPath());
					}

					return FReply::Handled();
				})
				[
					SAssignNew(ButtonContent, SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SVoxelDetailText)
						.Text(FText::FromString(FName::NameToDisplayString(Node->Name.ToString(), false)))
					]
				]
			]
		];

	ButtonContent->InsertSlot(bIsInput ? 0 : 1)
	.AutoWidth()
	[
		SNew(SImage)
		.Image_Lambda([=]() -> const FSlateBrush*
		{
			const TSet<FName>& CollapsedCategories = bIsInput ? GetVoxelNode().CollapsedInputCategories : GetVoxelNode().CollapsedOutputCategories;
			if (CollapsedCategories.Contains(Node->GetConcatenatedPath()))
			{
				if (Button->IsHovered())
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Collapsed_Hovered"));
				}
				else
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Collapsed"));
				}
			}
			else
			{
				if (Button->IsHovered())
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Expanded_Hovered"));
				}
				else
				{
					return FAppStyle::Get().GetBrush(STATIC_FNAME("TreeArrow_Expanded"));
				}
			}
		})
		.ColorAndOpacity(FSlateColor::UseForeground())
	];

	TSharedPtr<SWidget> ItemsNumWidget;
	TSharedPtr<SWidget> VariadicPinButtons;
	TSharedPtr<SWidget> NoEntriesWidget;
	CreateCategoryVariadicButtons(Node, bIsInput, ItemsNumWidget, VariadicPinButtons, NoEntriesWidget);

	if (ItemsNumWidget)
	{
		ButtonContent->InsertSlot(bIsInput ? 2 : 0)
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(bIsInput ? FMargin(3.f, 0.f, 0.f, 0.f) : FMargin(0.f, 0.f, 3.f, 0.f))
		[
			ItemsNumWidget.ToSharedRef()
		];
	}

	if (VariadicPinButtons)
	{
		NameRow->GetSlot(0).SetAutoWidth();
		NameRow->InsertSlot(bIsInput ? 1 : 0)
		.AutoWidth()
		[
			VariadicPinButtons.ToSharedRef()
		];
	}

	if (NoEntriesWidget)
	{
		FMargin Padding = bIsInput ? Settings->GetInputPinPadding() : Settings->GetOutputPinPadding();
		if (bIsInput)
		{
			Padding.Left *= 2.f;
		}
		else
		{
			Padding.Right *= 2.f;
		}

		Result->AddSlot()
		.Padding(Padding)
		.AutoHeight()
		.HAlign(bIsInput ? HAlign_Left : HAlign_Right)
		.VAlign(VAlign_Center)
		[
			NoEntriesWidget.ToSharedRef()
		];
	}

	return Result;
}

void SVoxelGraphNode::CreateCategoryVariadicButtons(
	const TSharedRef<IVoxelNodeDefinition::FNode>& Node,
	const bool bIsInput,
	TSharedPtr<SWidget>& OutItemsNumWidget,
	TSharedPtr<SWidget>& OutButtonsWidget,
	TSharedPtr<SWidget>& OutNoEntriesWidget) const
{
	if (!Node->IsVariadicPin())
	{
		return;
	}

	OutItemsNumWidget =
		SNew(SVoxelDetailText)
		.Text(FText::FromString("[" + LexToString(Node->GetChildren().Num()) + "]"))
		.ColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f));

	OutButtonsWidget =
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(bIsInput ? FMargin(5.f, 0.f, 0.f, 0.f) : FMargin(0.f, 0.f, 2.f, 0.f))
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.ToolTipText(INVTEXT("Remove pin"))
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.IsEnabled_Lambda([=]
			{
				return IsNodeEditable() && NodeDefinition->Variadic_CanRemovePinFrom(Node->Name);
			})
			.OnClicked_Lambda([=]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Remove pin");
				NodeDefinition->Variadic_RemovePinFrom(Node->Name);
				return FReply::Handled();
			})
			.Visibility_Lambda([=]
			{
				return GetButtonVisibility(NodeDefinition->Variadic_CanAddPinTo(Node->Name) || NodeDefinition->Variadic_CanRemovePinFrom(Node->Name));
			})
			[
				SNew(SImage)
				.Image(FVoxelEditorStyle::GetBrush(TEXT("Icons.MinusCircle")))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(bIsInput ? FMargin(2.f, 0.f, 0.f, 0.f) : FMargin(0.f, 0.f, 5.f, 0.f))
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.ToolTipText(INVTEXT("Add pin"))
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.IsEnabled_Lambda([=]
			{
				return IsNodeEditable() && NodeDefinition->Variadic_CanAddPinTo(Node->Name);
			})
			.OnClicked_Lambda([=]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Add pin");
				const FName NewPin = NodeDefinition->Variadic_AddPinTo(Node->Name);
				if (!NewPin.IsNone())
				{
					NodeDefinition->ExposePin(NewPin);
				}

				return FReply::Handled();
			})
			.Visibility_Lambda([=]
			{
				return GetButtonVisibility(NodeDefinition->Variadic_CanAddPinTo(Node->Name) || NodeDefinition->Variadic_CanRemovePinFrom(Node->Name));
			})
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
			]
		];

	if (Node->GetChildren().Num() > 0)
	{
		return;
	}

	OutNoEntriesWidget =
		SNew(SVoxelDetailText)
		.Visibility_Lambda([=]
		{
			return GetVoxelNode().IsVisible(*Node) ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.Text(INVTEXT("No entries"))
		.ColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f));
}

void SVoxelGraphNode::CreateAdvancedCategory(const TSharedPtr<SVerticalBox>& MainBox) const
{
	if (!bCreateAdvancedCategory)
	{
		return;
	}

	MainBox->AddSlot()
	.AutoHeight()
	.HAlign(HAlign_Fill)
	.VAlign(VAlign_Top)
	.Padding(3.f, 0.f, 3.f, 3.f)
	[
		SNew(SCheckBox)
		.OnCheckStateChanged_Lambda([this](ECheckBoxState)
		{
			GetVoxelNode().bShowAdvanced = !GetVoxelNode().bShowAdvanced;
		})
		.IsChecked_Lambda([this]
		{
			return GetVoxelNode().bShowAdvanced ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
		})
		.Cursor(EMouseCursor::Default)
		.Style(FAppStyle::Get(), "Graph.Node.AdvancedView")
		.ToolTipText_Lambda([this]
		{
			return GetVoxelNode().bShowAdvanced ? INVTEXT("Hide advanced pins") : INVTEXT("Show advanced pins");
		})
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SImage)
				.Image_Lambda([this]
				{
					return FAppStyle::GetBrush(GetVoxelNode().bShowAdvanced ? "Icons.ChevronUp" : "Icons.ChevronDown");
				})
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphNode::MakeStatWidget() const
{
	const TSharedRef<SVerticalBox> VBox =
		SNew(SVerticalBox)
		.Cursor(EMouseCursor::Default);

	const auto AddStatWidget = [&](IVoxelNodeStatProvider* Provider, UEdGraphPin* Pin)
	{
		TAttribute<FSlateColor> Color;
		if (Pin)
		{
			Color = MakeAttributeLambda([=]() -> FSlateColor
			{
				const FLinearColor Result = FLinearColor(FColor::Orange);
				return Result * (FindWidgetForPin(Pin)->IsHovered() ? 1.f : 0.6f);
			});
		}
		else
		{
			Color = FLinearColor(FColor::Orange) * 0.6f;
		}

		VBox->AddSlot()
		.AutoHeight()
		[
			SNew(SOverlay)
			.ToolTipText_Lambda([this, Provider, Pin]
			{
				return Pin ? Provider->GetPinToolTip(*Pin) : Provider->GetNodeToolTip(GetVoxelNode());
			})
			.Visibility_Lambda([this, Provider, Pin]
			{
				if (const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetVoxelNode().GetToolkit())
				{
					if (!Provider->IsEnabled(*Toolkit->Asset))
					{
						return EVisibility::Collapsed;
					}
				}
				else
				{
					return EVisibility::Collapsed;
				}

				const FText Text = Pin ? Provider->GetPinText(*Pin) : Provider->GetNodeText(GetVoxelNode());
				if (Text.IsEmpty())
				{
					return EVisibility::Collapsed;
				}

				return EVisibility::Visible;
			})
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SImage)
				.Image(FVoxelEditorStyle::GetBrush("Node.Stats.TitleGloss"))
				.ColorAndOpacity(Color)
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
				.BorderImage(FVoxelEditorStyle::GetBrush("Node.Stats.ColorSpill"))
				.Padding(FMargin(10.f, 5.f, 20.f, 5.f))
				.BorderBackgroundColor(Color)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.Padding(FMargin(0.f, 0.f, 4.f, 0.f))
					.AutoWidth()
					[
						SNew(SImage)
						.DesiredSizeOverride(FVector2D(16.f, 16.f))
						.Image(FSlateIcon(Provider->GetIconStyleSetName(), Provider->GetIconName()).GetIcon())
						.ColorAndOpacity(Color)
					]
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.MinDesiredWidth(55.f)
						[
							SNew(STextBlock)
							.ColorAndOpacity(FCoreStyle::Get().GetColor("ErrorReporting.ForegroundColor"))
							.Text_Lambda([this, Provider, Pin]
							{
								return Pin ? Provider->GetPinText(*Pin) : Provider->GetNodeText(GetVoxelNode());
							})
						]
					]
				]
			]
		];
	};

	for (IVoxelNodeStatProvider* Provider : GVoxelNodeStatProviders)
	{
		AddStatWidget(Provider, nullptr);

		for (UEdGraphPin* Pin : GraphNode->Pins)
		{
			if (Pin->Direction != EGPD_Output)
			{
				continue;
			}

			AddStatWidget(Provider, Pin);
		}
	}
	return VBox;
}

EVisibility SVoxelGraphNode::GetButtonVisibility(const bool bVisible) const
{
	if (SGraphNode::IsAddPinButtonVisible() == EVisibility::Collapsed)
	{
		// LOD is too low
		return EVisibility::Collapsed;
	}

	return bVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

void SVoxelGraphNode::CreateStandardNodeTitleButtons(const TSharedPtr<SHorizontalBox>& TitleBox)
{
	TSharedPtr<SHorizontalBox> Box;

	TitleBox->AddSlot()
	.FillWidth(1.f)
	.HAlign(HAlign_Right)
	.VAlign(VAlign_Center)
	.Padding(0.f, 1.f, 7.f, 0.f)
	[
		SAssignNew(Box, SHorizontalBox)
		+ SHorizontalBox::Slot()
		.Padding(0.f, 0.f, 2.f, 0.f)
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.IsEnabled_Lambda([this]
			{
				return IsNodeEditable() && NodeDefinition->CanRemoveInputPin();
			})
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ToolTipText(FText::FromString(NodeDefinition->GetRemovePinTooltip()))
			.OnClicked_Lambda([this]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Remove input pin");
				NodeDefinition->RemoveInputPin();
				return FReply::Handled();
			})
			.Visibility_Lambda([this]
			{
				return GetButtonVisibility(NodeDefinition->CanAddInputPin() || NodeDefinition->CanRemoveInputPin());
			})
			[
				SNew(SImage)
				.Image(FVoxelEditorStyle::GetBrush(TEXT("Icons.MinusCircle")))
			]
		]
		+ SHorizontalBox::Slot()
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.Cursor(EMouseCursor::Default)
			.IsEnabled_Lambda([this]
			{
				return IsNodeEditable() && NodeDefinition->CanAddInputPin();
			})
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ToolTipText(FText::FromString(NodeDefinition->GetAddPinTooltip()))
			.OnClicked_Lambda([this]
			{
				const FVoxelTransaction Transaction(GetVoxelNode(), "Add input pin");
				NodeDefinition->AddInputPin();
				return FReply::Handled();
			})
			.Visibility_Lambda([this]
			{
				return GetButtonVisibility(NodeDefinition->CanAddInputPin() || NodeDefinition->CanRemoveInputPin());
			})
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(TEXT("Icons.PlusCircle")))
			]
		]
	];

	UVoxelGraphNode_Struct* StructNode = Cast<UVoxelGraphNode_Struct>(&GetVoxelNode());
	if (!StructNode ||
		!StructNode->Struct)
	{
		return;
	}

	bool bShowOverlay = false;
	for (const FProperty& Property : GetStructProperties(StructNode->Struct->GetStruct()))
	{
		if (!Property.HasAnyPropertyFlags(CPF_Edit) ||
			Property.GetFName() == GET_MEMBER_NAME_CHECKED(FVoxelNode, ExposedPinValues))
		{
			continue;
		}

		bShowOverlay = true;
		break;
	}

	for (const FName PinName : StructNode->Struct->PrivatePinsOrder)
	{
		if (const TSharedPtr<FVoxelNode::FVariadicPin> VariadicPin = StructNode->Struct->PrivateNameToVariadicPin.FindRef(PinName))
		{
			if (VariadicPin->PinTemplate.Metadata.bShowInDetail)
			{
				bShowOverlay = true;
				break;
			}

			continue;
		}

		if (const TSharedPtr<FVoxelPin> VoxelPin = StructNode->Struct->FindPin(PinName))
		{
			if (VoxelPin->Metadata.bShowInDetail)
			{
				bShowOverlay = true;
				break;
			}
		}
	}

	if (!bShowOverlay)
	{
		return;
	}

	const FSlateBrush* ImageBrush = FAppStyle::GetBrush(TEXT("Kismet.Tabs.Palette"));
	const FLinearColor Color = FLinearColor::White;

	Box->AddSlot()
	.Padding(3.f, 0.f, 0.f, 0.f)
	[
		SNew(SImage)
		.Image(ImageBrush)
		.ToolTipText(INVTEXT("This node is configurable through the details panel. Select the node to make changes"))
		.Visibility(EVisibility::Visible)
		.ColorAndOpacity(Color)
	];
}