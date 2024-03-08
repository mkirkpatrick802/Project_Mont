// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Members/VoxelMembersSchemaAction_Function.h"
#include "SVoxelGraphMembers.h"
#include "VoxelGraphToolkit.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "Nodes/VoxelGraphNode_CallFunction.h"

void FVoxelMembersSchemaAction_Function::OnPaste(
	FVoxelGraphToolkit& Toolkit,
	const UVoxelTerminalGraph* TerminalGraphToCopy)
{
	if (!ensure(TerminalGraphToCopy))
	{
		return;
	}
	UVoxelGraph& Graph = *Toolkit.Asset;

	FGuid NewGuid = TerminalGraphToCopy->GetGuid();
	if (Graph.FindTerminalGraph_NoInheritance(NewGuid))
	{
		// Reset GUID if there's already a function with the same one
		NewGuid = FGuid::NewGuid();
	}

	UVoxelTerminalGraph* NewTerminalGraph;
	{
		const FVoxelTransaction Transaction(Graph, "Paste function");
		NewTerminalGraph = &Graph.AddTerminalGraph(NewGuid, TerminalGraphToCopy);
	}

	Toolkit.OpenGraphAndBringToFront(NewTerminalGraph->GetEdGraph(), true);
	Toolkit.GetSelection().SelectFunction(NewGuid);
	Toolkit.GetSelection().RequestRename();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UObject* FVoxelMembersSchemaAction_Function::GetOuter() const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return nullptr;
	}

	return &TerminalGraph->GetGraph();
}

TSharedRef<SWidget> FVoxelMembersSchemaAction_Function::CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const
{
	return SNew(SVoxelMembersPaletteItem_Function, CreateData);
}

void FVoxelMembersSchemaAction_Function::ApplyNewGuids(const TArray<FGuid>& NewGuids) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	UVoxelGraph& Graph = TerminalGraph->GetGraph();

	const FVoxelTransaction Transaction(Graph, "Reorder functions");
	Graph.ReorderTerminalGraphs(NewGuids);
}

FReply FVoxelMembersSchemaAction_Function::OnDragged(const FPointerEvent& MouseEvent)
{
	return FReply::Handled().BeginDragDrop(FVoxelMembersDragDropAction_Function::New(
		SharedThis(this),
		WeakTerminalGraph));
}

void FVoxelMembersSchemaAction_Function::BuildContextMenu(FMenuBuilder& MenuBuilder)
{
	if (!ensure(WeakTerminalGraph.IsValid()) ||
		WeakTerminalGraph->GetBaseTerminalGraphs().Num() < 2)
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		INVTEXT("Go to parent"),
		INVTEXT("Go to the parent function"),
		{},
		FUIAction
		{
			MakeLambdaDelegate([=]
			{
				const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
				if (!ensure(TerminalGraph))
				{
					return;
				}

				const TVoxelArray<const UVoxelTerminalGraph*> BaseTerminalGraphs = TerminalGraph->GetBaseTerminalGraphs().Array();
				check(BaseTerminalGraphs[0] == TerminalGraph);
				if (!BaseTerminalGraphs.IsValidIndex(1))
				{
					return;
				}

				FVoxelUtilities::FocusObject(BaseTerminalGraphs[1]);
			}),
		});
}

void FVoxelMembersSchemaAction_Function::OnActionDoubleClick() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	FVoxelUtilities::FocusObject(TerminalGraph);
}

void FVoxelMembersSchemaAction_Function::OnSelected() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!ensure(Toolkit))
	{
		return;
	}

	Toolkit->GetSelection().SelectFunction(MemberGuid);
}

void FVoxelMembersSchemaAction_Function::OnDelete() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	UVoxelGraph::LoadAllGraphs();

	TVoxelArray<UVoxelGraphNode_CallFunction*> FunctionNodes;
	if (TerminalGraph->IsTopmostTerminalGraph())
	{
		const FGuid Guid = TerminalGraph->GetGuid();

		ForEachObjectOfClass<UVoxelGraphNode_CallFunction>([&](UVoxelGraphNode_CallFunction& FunctionNode)
		{
			if (FunctionNode.Guid != Guid)
			{
				return;
			}

			const UVoxelGraph* Graph = FunctionNode.GetTypedOuter<UVoxelGraph>();
			if (!ensure(Graph) ||
				!Graph->GetBaseGraphs().Contains(&TerminalGraph->GetGraph()))
			{
				return;
			}

			FunctionNodes.Add(&FunctionNode);
		});
	}

	EResult DeleteNodes = EResult::No;
	if (FunctionNodes.Num() > 0)
	{
		DeleteNodes = CreateDeletePopups("Delete function", TerminalGraph->GetDisplayName());

		if (DeleteNodes == EResult::Cancel)
		{
			return;
		}
	}

	Toolkit->CloseGraph(TerminalGraph->GetEdGraph());

	{
		UVoxelGraph& Graph = TerminalGraph->GetGraph();
		const FVoxelTransaction Transaction(Graph, "Delete function");

		if (DeleteNodes == EResult::Yes)
		{
			for (UVoxelGraphNode_CallFunction* Node : FunctionNodes)
			{
				UEdGraph* NodeGraph = Node->GetGraph();
				if (!ensure(NodeGraph))
				{
					continue;
				}

				NodeGraph->RemoveNode(Node);
			}
		}
		else
		{
			// Reset references
			for (UVoxelGraphNode_CallFunction* Node : FunctionNodes)
			{
				Node->Guid = {};
				Node->ReconstructNode();
			}
		}

		ensure(Graph.FindTerminalGraph(MemberGuid) == TerminalGraph);
		Graph.RemoveTerminalGraph(MemberGuid);
		TerminalGraph->MarkAsGarbage();
	}

	MarkAsDeleted();

	if (const TSharedPtr<SVoxelGraphMembers> Members = GetMembers())
	{
		Members->SelectClosestAction(GetSectionID(), MemberGuid);
	}
}

void FVoxelMembersSchemaAction_Function::OnDuplicate() const
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	OnPaste(*Toolkit, TerminalGraph);
}

bool FVoxelMembersSchemaAction_Function::OnCopy(FString& OutText) const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	OutText = TerminalGraph->GetPathName();
	return true;
}

bool FVoxelMembersSchemaAction_Function::CanBeRenamed() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	return TerminalGraph->IsTopmostTerminalGraph();
}

FString FVoxelMembersSchemaAction_Function::GetName() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!TerminalGraph)
	{
		return {};
	}

	return TerminalGraph->GetDisplayName();
}

void FVoxelMembersSchemaAction_Function::SetName(const FString& Name) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Rename function");

	TerminalGraph->UpdateMetadata([&](FVoxelGraphMetadata& Metadata)
	{
		Metadata.DisplayName = Name;
	});
}

void FVoxelMembersSchemaAction_Function::SetCategory(const FString& NewCategory) const
{
	UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const FVoxelTransaction Transaction(TerminalGraph, "Set function category");

	TerminalGraph->UpdateMetadata([&](FVoxelGraphMetadata& Metadata)
	{
		Metadata.Category = NewCategory;
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelMembersPaletteItem_Function::Construct(const FArguments& InArgs, const FCreateWidgetForActionData& CreateData)
{
	ActionPtr = CreateData.Action;

	ChildSlot
	[
		SNew(SBox)
		.Padding(FMargin(0.f, -2.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.Padding(FMargin(0.f, 2.f, 4.f, 2.f))
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(TEXT("GraphEditor.Function_16x")))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f)
			[
				CreateTextSlotWidget(CreateData)
			]
			+ SHorizontalBox::Slot()
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(3.f)
			[
				SNew(STextBlock)
				.Visibility_Lambda([this]
				{
					const TSharedPtr<FEdGraphSchemaAction> Action = ActionPtr.Pin();
					if (!Action)
					{
						return EVisibility::Collapsed;
					}

					const UVoxelTerminalGraph* TerminalGraph = static_cast<FVoxelMembersSchemaAction_Function&>(*Action).WeakTerminalGraph.Get();
					if (!TerminalGraph)
					{
						return EVisibility::Collapsed;
					}

					return TerminalGraph->IsTopmostTerminalGraph() ? EVisibility::Collapsed : EVisibility::Visible;
				})
				.Text(INVTEXT("override"))
				.ToolTipText(INVTEXT("This function is an override"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		]
	];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMembersDragDropAction_Function::HoverTargetChanged()
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	const UEdGraph* EdGraph = GetHoveredGraph();
	if (!EdGraph)
	{
		FVoxelMembersDragDropAction_Base::HoverTargetChanged();
		return;
	}

	if (TerminalGraph->GetTypedOuter<UVoxelGraph>() != EdGraph->GetTypedOuter<UVoxelGraph>() &&
		!TerminalGraph->GetTypedOuter<UVoxelGraph>()->IsFunctionLibrary())
	{
		SetFeedbackMessageError("Cannot use in a different graph");
		return;
	}

	FVoxelMembersDragDropAction_Base::HoverTargetChanged();
}

FReply FVoxelMembersDragDropAction_Function::DroppedOnPanel(
	const TSharedRef<SWidget>& Panel,
	const FVector2D ScreenPosition,
	const FVector2D GraphPosition,
	UEdGraph& EdGraph)
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return FReply::Unhandled();
	}

	if (TerminalGraph->GetTypedOuter<UVoxelGraph>() != EdGraph.GetTypedOuter<UVoxelGraph>() &&
		!TerminalGraph->GetTypedOuter<UVoxelGraph>()->IsFunctionLibrary())
	{
		return FReply::Unhandled();
	}

	FVoxelGraphSchemaAction_NewCallFunctionNode Action;
	Action.Guid = TerminalGraph->GetGuid();
	Action.Apply(EdGraph, GraphPosition, true);

	return FReply::Handled();
}