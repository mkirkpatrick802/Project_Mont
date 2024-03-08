// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMessageToken_Callstack.h"
#include "VoxelGraph.h"
#include "VoxelCallstack.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Logging/TokenizedMessage.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#endif

uint32 FVoxelMessageToken_Callstack::GetHash() const
{
	return Callstack->GetHash();
}

FString FVoxelMessageToken_Callstack::ToString() const
{
	return "\nCallstack: " + Callstack->ToDebugString();
}

TSharedRef<IMessageToken> FVoxelMessageToken_Callstack::GetMessageToken() const
{
#if WITH_EDITOR
	return FActionToken::Create(
		INVTEXT("View Callstack"),
		INVTEXT("View callstack"),
		MakeLambdaDelegate([Callstack = Callstack]
		{
			const TSharedRef<SVoxelCallstack> CallstackWidget =
				SNew(SVoxelCallstack)
				.Callstack(Callstack);

			const FDockTabStyle& TabStyle = FCoreStyle::Get().GetWidgetStyle<FDockTabStyle>("Docking.Tab");

			const TSharedRef<SWindow> Window =
				SNew(SWindow)
				.Type(EWindowType::Menu)
				.SupportsMaximize(false)
				.SupportsMinimize(false)
				.IsPopupWindow(true)
				.bDragAnywhere(true)
				.IsTopmostWindow(true)
				.SizingRule(ESizingRule::Autosized)
				.SupportsTransparency(EWindowTransparency::PerPixel);

			Window->SetContent(
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					CallstackWidget
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Top)
				[
					SNew(SButton)
					.ButtonStyle(&TabStyle.CloseButtonStyle)
					.OnClicked_Lambda([WeakNotificationWindow = MakeWeakPtr(Window)]
					{
						const TSharedPtr<SWindow> PinnedNotificationWindow = WeakNotificationWindow.Pin();
						if (!ensure(PinnedNotificationWindow))
						{
							return FReply::Handled();
						}

						PinnedNotificationWindow->RequestDestroyWindow();
						return FReply::Handled();
					})
				]);

			const TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
			if (!ensure(RootWindow))
			{
				return;
			}

			FSlateApplication::Get().AddWindowAsNativeChild(Window, RootWindow.ToSharedRef());
			Window->BringToFront();
		}));
#else
	return Super::GetMessageToken();
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void SVoxelCallstack::Construct(const FArguments& Args)
{
	SetCursor(EMouseCursor::CardinalCross);

	const TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

	TVoxelArray<const FVoxelCallstack*> Callstacks;
	for (const FVoxelCallstack* Callstack = Args._Callstack.Get(); Callstack; Callstack = Callstack->GetParent())
	{
		const FVoxelGraphNodeRef NodeRef = Callstack->GetNodeRef();
		if (NodeRef.IsExplicitlyNull())
		{
			continue;
		}

		if (Callstacks.Num() > 0 &&
			Callstacks.Last()->GetNodeRef() == NodeRef)
		{
			if (!Callstacks.Last()->GetPinName().IsValid())
			{
				Callstacks.Last() = Callstack;
			}
			continue;
		}

		Callstacks.Add(Callstack);
	}
	int32 Num = Callstacks.Num() - 1;

	const UObject* LastRuntimeProvider = nullptr;
	const UVoxelTerminalGraph* LastTerminalGraph = nullptr;

	struct FItem
	{
		FName PinName;
		FVoxelGraphNodeRef NodeRef;
	};
	TVoxelArray<FItem> PendingItems;

	const auto FlushNodes = [&]
	{
		if (PendingItems.Num() == 0)
		{
			return;
		}

		FString RuntimeName = CastChecked<UObject>(LastRuntimeProvider)->GetName();
		if (const AActor* Actor = Cast<AActor>(LastRuntimeProvider))
		{
			RuntimeName = Actor->GetActorLabel();
		}
		if (const UActorComponent* Component = Cast<UActorComponent>(LastRuntimeProvider))
		{
			RuntimeName = Component->GetOwner()->GetActorLabel() + "." + Component->GetName();
		}

		FString GraphName = LastTerminalGraph->GetGraph().GetName();
		if (!LastTerminalGraph->IsMainTerminalGraph())
		{
			GraphName += "." + LastTerminalGraph->GetDisplayName();
		}

		VerticalBox->AddSlot()
		.Padding(0, 10.f, 0.f, 5.f)
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(INVTEXT("Actor: "))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SHyperlink)
				.Text(FText::FromString(RuntimeName))
				.TextStyle(FAppStyle::Get(), "MessageLog")
				.OnNavigate_Lambda([=]
                {
	                FVoxelUtilities::FocusObject(LastRuntimeProvider);
                })
			]
			+ SHorizontalBox::Slot()
			  .Padding(10.f, 0.f, 0.f, 0.f)
			  .AutoWidth()
			[
				SNew(STextBlock)
				.Text(INVTEXT("Graph: "))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SHyperlink)
				.Text(FText::FromString(GraphName))
				.TextStyle(FAppStyle::Get(), "MessageLog")
				.OnNavigate_Lambda([=]
				{
					FVoxelUtilities::FocusObject(LastTerminalGraph);
				})
			]
		];

		for (const FItem& Item : PendingItems)
		{
			VerticalBox->AddSlot()
			.AutoHeight()
			.Padding(0, 5.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(5.f, 0.f, 10.f, 0.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::FromInt(Num--)))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SHyperlink)
					.Text(FText::FromName(Item.NodeRef.EdGraphNodeTitle))
					.TextStyle(FAppStyle::Get(), "MessageLog")
					.OnNavigate_Lambda([=]
					{
						const UEdGraphNode* GraphNode = Item.NodeRef.GetGraphNode_EditorOnly();
						if (!ensureVoxelSlow(GraphNode))
						{
							return;
						}

						FVoxelUtilities::FocusObject(GraphNode);
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(STextBlock)
					.TextStyle(FAppStyle::Get(), "HintText")
					.Text(FText::FromString("\tquerying " + Item.PinName.ToString()))
					.Visibility(Item.PinName.IsNone() ? EVisibility::Collapsed : EVisibility::Visible)
				]
			];
		}

		LastRuntimeProvider = nullptr;
		LastTerminalGraph = nullptr;
		PendingItems.Reset();
	};

	for (const FVoxelCallstack* Callstack : Callstacks)
	{
		const UObject* RuntimeProvider = Callstack->GetRuntimeProvider().Get();
		const UVoxelTerminalGraph* TerminalGraph = Callstack->GetNodeRef().GetNodeTerminalGraph(FOnVoxelGraphChanged::Null());
		if (!ensureVoxelSlow(RuntimeProvider) ||
			!ensureVoxelSlow(TerminalGraph))
		{
			continue;
		}

		if (LastRuntimeProvider != RuntimeProvider ||
			LastTerminalGraph != TerminalGraph)
		{
			FlushNodes();

			LastTerminalGraph = TerminalGraph;
			LastRuntimeProvider = RuntimeProvider;
		}

		PendingItems.Add(FItem
			{
				Callstack->GetPinName(),
				Callstack->GetNodeRef()
			});
	}

	FlushNodes();

	ChildSlot
	[
		SNew(SBox)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush("Menu.Background"))
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetOptionalBrush("Menu.Outline", nullptr))
			]
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.Padding(5.f, 0.f)
				.BorderImage(FStyleDefaults::GetNoBrush())
				.ForegroundColor(FCoreStyle::Get().GetSlateColor("DefaultForeground"))
				[
					SNew(SBox)
					.MaxDesiredHeight(720)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						.Padding(5.f, 0.f)
						[
							VerticalBox
						]
					]
				]
			]
		]
	];
}
#endif