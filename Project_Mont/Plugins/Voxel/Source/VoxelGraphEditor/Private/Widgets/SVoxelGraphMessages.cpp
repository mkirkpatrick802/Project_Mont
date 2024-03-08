// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "SVoxelGraphMessages.h"
#include "VoxelGraph.h"
#include "VoxelMessage.h"
#include "VoxelTerminalGraph.h"
#include "SVoxelNotification.h"
#include "VoxelMessageTokens.h"
#include "Nodes/VoxelGraphNode.h"
#include "VoxelGraphMessageTokens.h"
#include "VoxelTerminalGraphRuntime.h"

void SVoxelGraphMessages::Construct(const FArguments& Args)
{
	ensure(Args._Graph);
	WeakGraph = Args._Graph;

	ChildSlot
	[
		SNew(SScrollBox)
		.Orientation(Orient_Horizontal)
		+ SScrollBox::Slot()
		.FillSize(1.f)
		[
			SAssignNew(Tree, STree)
			.TreeItemsSource(&Nodes)
			.OnGenerateRow_Lambda([=](const TSharedPtr<INode>& Node, const TSharedRef<STableViewBase>& OwnerTable)
			{
				return
					SNew(STableRow<TSharedPtr<INode>>, OwnerTable)
					[
						Node->GetWidget()
					];
			})
			.OnGetChildren_Lambda([=](const TSharedPtr<INode>& Node, TArray<TSharedPtr<INode>>& OutChildren)
			{
				OutChildren = Node->GetChildren();
			})
			.SelectionMode(ESelectionMode::None)
		]
	];

	Tree->SetItemExpansion(CompileNode, true);
	Tree->SetItemExpansion(RuntimeNode, true);

	UpdateNodes();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelGraphMessages::UpdateNodes()
{
	VOXEL_FUNCTION_COUNTER();

	UVoxelGraph* RootGraph = WeakGraph.Get();
	if (!ensure(RootGraph))
	{
		return;
	}

	MessageToCanonMessage.Reset();
	HashToCanonMessage.Reset();

	TSet<TSharedPtr<INode>> VisitedNodes;
	TSet<TSharedPtr<INode>> NodesToExpand;

	RootGraph->ForeachTerminalGraph_NoInheritance([&](UVoxelTerminalGraph& TerminalGraph)
	{
		for (UEdGraphNode* GraphNode : TerminalGraph.GetEdGraph().Nodes)
		{
			if (!GraphNode->bHasCompilerMessage)
			{
				continue;
			}

			GraphNode->bHasCompilerMessage = false;
			GraphNode->ErrorType = EMessageSeverity::Info;
			GraphNode->ErrorMsg.Reset();

			UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(GraphNode);
			if (!ensure(VoxelNode))
			{
				continue;
			}

			VoxelNode->RefreshNode();
		}

		const UVoxelTerminalGraphRuntime& Runtime = TerminalGraph.GetRuntime();

		TSharedPtr<FGraphNode>& CompileGraphNode = CompileNode->GraphToNode.FindOrAdd(&TerminalGraph);
		if (!CompileGraphNode)
		{
			CompileGraphNode = MakeVoxelShared<FGraphNode>(&TerminalGraph);
			NodesToExpand.Add(CompileGraphNode);
		}

		TSharedPtr<FGraphNode>& RuntimeGraphNode = RuntimeNode->GraphToNode.FindOrAdd(&TerminalGraph);
		if (!RuntimeGraphNode)
		{
			RuntimeGraphNode = MakeVoxelShared<FGraphNode>(&TerminalGraph);
			NodesToExpand.Add(RuntimeGraphNode);
		}

		for (const TSharedRef<FVoxelMessage>& Message : Runtime.GetCompileMessages())
		{
			VisitedNodes.Add(CompileGraphNode);

			for (const TSharedRef<FVoxelMessageToken>& Token : Message->GetTokens())
			{
				UVoxelGraphNode* GraphNode = nullptr;
				if (const TSharedPtr<FVoxelMessageToken_NodeRef> TypedToken = Cast<FVoxelMessageToken_NodeRef>(Token))
				{
					GraphNode = Cast<UVoxelGraphNode>(TypedToken->NodeRef.GetGraphNode_EditorOnly());
				}
				if (const TSharedPtr<FVoxelMessageToken_PinRef> TypedToken = Cast<FVoxelMessageToken_PinRef>(Token))
				{
					GraphNode = Cast<UVoxelGraphNode>(TypedToken->PinRef.NodeRef.GetGraphNode_EditorOnly());
				}
				if (const TSharedPtr<FVoxelMessageToken_Pin> TypedToken = Cast<FVoxelMessageToken_Pin>(Token))
				{
					if (const UEdGraphPin* Pin = TypedToken->PinReference.Get())
					{
						GraphNode = Cast<UVoxelGraphNode>(Pin->GetOwningNodeUnchecked());
					}
				}

				if (!GraphNode ||
					// Try not to leak errors
					GraphNode->GetTypedOuter<UVoxelTerminalGraph>() != &TerminalGraph)
				{
					continue;
				}

				GraphNode->bHasCompilerMessage = true;
				GraphNode->ErrorType = FMath::Min<int32>(GraphNode->ErrorType, Message->GetMessageSeverity());

				if (!GraphNode->ErrorMsg.IsEmpty())
				{
					GraphNode->ErrorMsg += "\n";
				}
				GraphNode->ErrorMsg += Message->ToString();

				GraphNode->RefreshNode();
			}

			TSharedPtr<FMessageNode>& MessageNode = CompileGraphNode->MessageToNode.FindOrAdd(Message);
			if (!MessageNode)
			{
				MessageNode = MakeVoxelShared<FMessageNode>(Message);
				NodesToExpand.Add(MessageNode);
			}
			VisitedNodes.Add(MessageNode);
		}

		for (const TSharedRef<FVoxelMessage>& Message : Runtime.GetRuntimeMessages())
		{
			VisitedNodes.Add(RuntimeGraphNode);

			const TSharedRef<FVoxelMessage> CanonMessage = GetCanonMessage(Message);

			TSharedPtr<FMessageNode>& MessageNode = RuntimeGraphNode->MessageToNode.FindOrAdd(CanonMessage);
			if (!MessageNode)
			{
				MessageNode = MakeVoxelShared<FMessageNode>(CanonMessage);
				NodesToExpand.Add(MessageNode);
			}
			VisitedNodes.Add(MessageNode);
		}

		for (const auto& It : Runtime.GetPinNameToCompileMessages())
		{
			if (It.Value.Num() == 0)
			{
				continue;
			}

			VisitedNodes.Add(CompileGraphNode);

			TSharedPtr<FPinNode>& PinNode = CompileGraphNode->PinNameToNode.FindOrAdd(It.Key);
			if (!PinNode)
			{
				PinNode = MakeVoxelShared<FPinNode>(It.Key);
				NodesToExpand.Add(PinNode);
			}
			VisitedNodes.Add(PinNode);

			for (const TSharedRef<FVoxelMessage>& Message : It.Value)
			{
				TSharedPtr<FMessageNode>& MessageNode = PinNode->MessageToNode.FindOrAdd(Message);
				if (!MessageNode)
				{
					MessageNode = MakeVoxelShared<FMessageNode>(Message);
					NodesToExpand.Add(MessageNode);
				}
				VisitedNodes.Add(MessageNode);
			}
		}
	});

	const auto Cleanup = [&](FRootNode& RootNode)
	{
		for (auto GraphNodeIt = RootNode.GraphToNode.CreateIterator(); GraphNodeIt; ++GraphNodeIt)
		{
			if (!VisitedNodes.Contains(GraphNodeIt.Value()))
			{
				GraphNodeIt.RemoveCurrent();
				continue;
			}

			// Put errors on top
			const auto SortPredicate = [](const TSharedPtr<FVoxelMessage>& A, const TSharedPtr<FVoxelMessage>& B)
			{
				return A->GetMessageSeverity() < B->GetMessageSeverity();
			};

			GraphNodeIt.Value()->MessageToNode.KeySort(SortPredicate);

			for (auto MessageIt = GraphNodeIt.Value()->MessageToNode.CreateIterator(); MessageIt; ++MessageIt)
			{
				if (!VisitedNodes.Contains(MessageIt.Value()))
				{
					MessageIt.RemoveCurrent();
				}
			}

			for (auto PinIt = GraphNodeIt.Value()->PinNameToNode.CreateIterator(); PinIt; ++PinIt)
			{
				if (!VisitedNodes.Contains(PinIt.Value()))
				{
					PinIt.RemoveCurrent();
					continue;
				}

				PinIt.Value()->MessageToNode.KeySort(SortPredicate);

				for (auto MessageIt = PinIt.Value()->MessageToNode.CreateIterator(); MessageIt; ++MessageIt)
				{
					if (!VisitedNodes.Contains(MessageIt.Value()))
					{
						MessageIt.RemoveCurrent();
					}
				}
			}
		}
	};

	Cleanup(*CompileNode);
	Cleanup(*RuntimeNode);

	// Always expand new items
	if (!Nodes.Contains(CompileNode))
	{
		NodesToExpand.Add(CompileNode);
	}
	if (!Nodes.Contains(RuntimeNode))
	{
		NodesToExpand.Add(RuntimeNode);
	}

	Nodes.Reset();

	if (CompileNode->GraphToNode.Num() > 0)
	{
		Nodes.Add(CompileNode);
	}
	if (RuntimeNode->GraphToNode.Num() > 0)
	{
		Nodes.Add(RuntimeNode);
	}

	for (const TSharedPtr<INode>& Node : NodesToExpand)
	{
		Tree->SetItemExpansion(Node, true);
	}

	Tree->RequestTreeRefresh();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphMessages::FRootNode::GetWidget() const
{
	return SNew(STextBlock).Text(FText::FromString(Text));
}

TArray<TSharedPtr<SVoxelGraphMessages::INode>> SVoxelGraphMessages::FRootNode::GetChildren() const
{
	TArray<TSharedPtr<FGraphNode>> Children;
	GraphToNode.GenerateValueArray(Children);
	return TArray<TSharedPtr<INode>>(Children);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphMessages::FGraphNode::GetWidget() const
{
	return SNew(STextBlock).Text_Lambda([Graph = Graph]
	{
		if (!Graph.IsValid())
		{
			return INVTEXT("Invalid");
		}

		if (Graph->IsMainTerminalGraph())
		{
			return INVTEXT("Main Graph");
		}

		return FText::FromString(Graph->GetDisplayName());
	});
}

TArray<TSharedPtr<SVoxelGraphMessages::INode>> SVoxelGraphMessages::FGraphNode::GetChildren() const
{
	TArray<TSharedPtr<INode>> Children;
	for (const auto& It : MessageToNode)
	{
		Children.Add(It.Value);
	}
	for (const auto& It : PinNameToNode)
	{
		Children.Add(It.Value);
	}
	return Children;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphMessages::FPinNode::GetWidget() const
{
	return SNew(STextBlock).Text(FText::FromString(PinName.ToString()));
}

TArray<TSharedPtr<SVoxelGraphMessages::INode>> SVoxelGraphMessages::FPinNode::GetChildren() const
{
	TArray<TSharedPtr<INode>> Children;
	for (const auto& It : MessageToNode)
	{
		Children.Add(It.Value);
	}
	return Children;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<SWidget> SVoxelGraphMessages::FMessageNode::GetWidget() const
{
	return SNew(SVoxelNotification, Message);
}

TArray<TSharedPtr<SVoxelGraphMessages::INode>> SVoxelGraphMessages::FMessageNode::GetChildren() const
{
	return {};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelMessage> SVoxelGraphMessages::GetCanonMessage(const TSharedRef<FVoxelMessage>& Message)
{
	if (const TSharedPtr<FVoxelMessage> CanonMessage = MessageToCanonMessage.FindRef(Message).Pin())
	{
		return CanonMessage.ToSharedRef();
	}

	TWeakPtr<FVoxelMessage>& WeakCanonMessage = HashToCanonMessage.FindOrAdd(Message->GetHash());

	if (const TSharedPtr<FVoxelMessage> CanonMessage = WeakCanonMessage.Pin())
	{
		MessageToCanonMessage.Add_EnsureNew(Message, CanonMessage);
		return CanonMessage.ToSharedRef();
	}

	WeakCanonMessage = Message;
	MessageToCanonMessage.Add_EnsureNew(Message, Message);
	return Message;
}