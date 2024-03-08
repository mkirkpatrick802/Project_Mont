// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTerminalGraphRuntime.h"
#include "VoxelGraph.h"
#include "VoxelBuffer.h"
#include "VoxelMessage.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphCompiler.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelCompilationGraph.h"
#include "VoxelGraphCompileScope.h"
#include "Nodes/VoxelNode_Root.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelDisableReconstructOnError, false,
	"voxel.DisableReconstructOnError",
	"");


#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(RegisterOnMessageLogged)
{
	GVoxelMessageManager->OnMessageLogged.AddLambda([](const TSharedRef<FVoxelMessage>& Message)
	{
		check(IsInGameThread());

		for (const UVoxelTerminalGraph* TerminalGraph : Message->GetTypedOuters<UVoxelTerminalGraph>())
		{
			TerminalGraph->GetRuntime().AddMessage(Message);
		}
	});
}
#endif

#if WITH_EDITOR
IVoxelGraphEditorInterface* GVoxelGraphEditorInterface = nullptr;
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const UVoxelGraph& UVoxelTerminalGraphRuntime::GetGraph() const
{
	return GetTerminalGraph().GetGraph();
}

UVoxelTerminalGraph& UVoxelTerminalGraphRuntime::GetTerminalGraph()
{
	return *GetOuterUVoxelTerminalGraph();
}

const UVoxelTerminalGraph& UVoxelTerminalGraphRuntime::GetTerminalGraph() const
{
	return *GetOuterUVoxelTerminalGraph();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FVoxelSerializedGraph& UVoxelTerminalGraphRuntime::GetSerializedGraph() const
{
#if WITH_EDITOR
	if (!PrivateSerializedGraph.bIsValid ||
		PrivateSerializedGraph.Version != FVoxelSerializedGraph::FVersion::LatestVersion)
	{
		ConstCast(this)->Translate();
	}
#endif

	return PrivateSerializedGraph;
}

void UVoxelTerminalGraphRuntime::EnsureIsCompiled()
{
	VOXEL_FUNCTION_COUNTER();

	if (!CachedCompiledGraph.IsSet())
	{
		CachedCompiledGraph = Compile();
	}
}


TSharedPtr<FVoxelGraphEvaluator> UVoxelTerminalGraphRuntime::CreateEvaluator(const FVoxelGraphPinRef& GraphPinRef)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(GraphPinRef.GetNodeTerminalGraph(FOnVoxelGraphChanged::Null()) == &GetTerminalGraph()))
	{
		return nullptr;
	}

	EnsureIsCompiled();

	const TSharedPtr<const Voxel::Graph::FGraph> CompiledGraph = CachedCompiledGraph.GetValue();
	if (!CompiledGraph)
	{
		return nullptr;
	}

	return CreateEvaluatorImpl(
		GraphPinRef.NodeRef.NodeId,
		GraphPinRef.PinName,
		*CompiledGraph);
}

#if WITH_EDITOR
void UVoxelTerminalGraphRuntime::BindOnEdGraphChanged()
{
	if (OnEdGraphChangedPtr)
	{
		// Already bound
		return;
	}

	OnEdGraphChangedPtr = MakeSharedVoid();

	GVoxelGraphTracker->OnEdGraphChanged(GetTerminalGraph().GetEdGraph()).Add(FOnVoxelGraphChanged::Make(OnEdGraphChangedPtr, this, [=]
	{
		Translate();
	}));
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelTerminalGraphRuntime::HasNode(const UScriptStruct* Struct) const
{
	VOXEL_FUNCTION_COUNTER();

#if WITH_EDITOR
	// Bypass in editor to avoid recursively migrating
	check(GVoxelGraphEditorInterface);
	return GVoxelGraphEditorInterface->HasNode(GetTerminalGraph(), Struct);
#endif

	for (const auto& It : GetSerializedGraph().NodeNameToNode)
	{
		if (It.Value.VoxelNode &&
			It.Value.VoxelNode.GetScriptStruct()->IsChildOf(Struct))
		{
			return true;
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelTerminalGraphRuntime::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();
	using namespace Voxel::Graph;

	const TSharedPtr<const FGraph> Graph = CastChecked<UVoxelTerminalGraphRuntime>(InThis)->CachedCompiledGraph.Get(nullptr);
	if (!Graph)
	{
		return;
	}

	for (const FNode& Node : Graph->GetNodes())
	{
		ConstCast(Node.GetVoxelNode()).AddStructReferencedObjects(Collector);

		for (const FPin& Pin : Node.GetInputPins())
		{
			FVoxelUtilities::AddStructReferencedObjects(
				Collector,
				MakeVoxelStructView(ConstCast(Pin.GetDefaultValue())));
		}
	}
}

void UVoxelTerminalGraphRuntime::Serialize(FArchive& Ar)
{
	VOXEL_FUNCTION_COUNTER();

	if (Ar.IsCooking())
	{
		// Make sure serialized graph is up-to-date & raise compile errors
		EnsureIsCompiled();
	}

	Super::Serialize(Ar);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelTerminalGraphRuntime::AddMessage(const TSharedRef<FVoxelMessage>& Message)
{
	check(IsInGameThread());

	if (GVoxelGraphCompileScope)
	{
		// Will be logged as compile error
		return;
	}

	RuntimeMessages.Add(Message);
	OnMessagesChanged.Broadcast();
}

bool UVoxelTerminalGraphRuntime::HasCompileMessages() const
{
	// Meaningless otherwise
	ensure(CachedCompiledGraph.IsSet());

	return
		CompileMessages.Num() > 0 ||
		PinNameToCompileMessages.Num() > 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelTerminalGraphRuntime::Translate()
{
	VOXEL_FUNCTION_COUNTER();
	check(GVoxelGraphEditorInterface);

	const FVoxelSerializedGraph OldSerializedGraph = MoveTemp(PrivateSerializedGraph);

	PrivateSerializedGraph = GVoxelGraphEditorInterface->Translate(GetTerminalGraph());

	if (OldSerializedGraph.bIsValid ||
		PrivateSerializedGraph.bIsValid)
	{
		// Avoid infinite notify loop, never notify if we were invalid & still are
		GVoxelGraphTracker->NotifyTerminalGraphTranslated(GetTerminalGraph());
	}

	// Clear runtime messages after translating as they're likely out of date
	RuntimeMessages.Reset();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<const Voxel::Graph::FGraph> UVoxelTerminalGraphRuntime::Compile()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	using namespace Voxel::Graph;

	// Clear pin ref messages - since we're recompiling the entire graph, they'll be recompiled too
	// This ensures there's no leftover messages
	PinNameToCompileMessages.Reset();

#if WITH_EDITOR
	OnTranslatedPtr = MakeSharedVoid();

	const FOnVoxelGraphChanged OnTranslated = FOnVoxelGraphChanged::Make(OnTranslatedPtr, this, [this]
	{
		CachedCompiledGraph.Reset();

		// Compile again to raise errors
		EnsureIsCompiled();

		GVoxelGraphTracker->NotifyTerminalGraphCompiled(GetTerminalGraph());
	});

	GVoxelGraphTracker->OnTerminalGraphTranslated(GetTerminalGraph()).Add(OnTranslated);
#else
	const FOnVoxelGraphChanged OnTranslated = FOnVoxelGraphChanged::Null();
#endif

	TSharedPtr<const FGraph> CompiledGraph = Compile(
		OnTranslated,
		GetTerminalGraph(),
		// In the editor the second Compile will handle error logging
		!WITH_EDITOR,
		CompileMessages);

#if WITH_EDITOR
	if (!CompiledGraph &&
		!GVoxelDisableReconstructOnError)
	{
		// Mute all notifications to avoid an infinite compile loop
		GVoxelGraphTracker->Mute();

		// Try reconstructing if we errored out
		check(GVoxelGraphEditorInterface);
		GVoxelGraphEditorInterface->ReconstructAllNodes(GetTerminalGraph());

		// Force translate now
		Translate();

		CompiledGraph = Compile(
			OnTranslated,
			GetTerminalGraph(),
			// Always log errors
			true,
			CompileMessages);

		GVoxelGraphTracker->Unmute();
	}
#endif

	// We updated the compile messages
	OnMessagesChanged.Broadcast();

	return CompiledGraph;
}

TSharedPtr<FVoxelGraphEvaluator> UVoxelTerminalGraphRuntime::CreateEvaluatorImpl(
	const FName NodeId,
	const FName PinName,
	const Voxel::Graph::FGraph& CompiledGraph)
{
	VOXEL_SCOPE_COUNTER_FORMAT("UVoxelRuntimeGraph::CreateEvaluator %s.%s", *NodeId.ToString(), *PinName.ToString());
	check(IsInGameThread());
	using namespace Voxel::Graph;

	const TSharedRef<FVoxelGraphCompiler> GraphCompiler = MakeVoxelShared<FVoxelGraphCompiler>(GetTerminalGraph());
	CompiledGraph.CloneInto(*GraphCompiler);

	FNode* TargetNode = nullptr;
	for (FNode& Node : GraphCompiler->GetNodes())
	{
		if (Node.NodeRef.NodeId != NodeId)
		{
			continue;
		}

		ensure(!TargetNode);
		TargetNode = &Node;
	}

	if (!TargetNode)
	{
		// Don't raise error for deleted nodes
		// We can't always detect them (eg DebugNode being deleted)
		return MakeVoxelShared<FVoxelGraphEvaluator>();
	}

	const FVoxelGraphCompileScope Scope(*GetOuterUVoxelTerminalGraph());
	ON_SCOPE_EXIT
	{
		const FName Name = TargetNode->NodeRef.EdGraphNodeTitle + TEXTVIEW(".") + PinName;

		PinNameToCompileMessages.Remove(Name);

		if (Scope.GetMessages().Num() > 0)
		{
			PinNameToCompileMessages.Add(Name, Scope.GetMessages());
		}

		OnMessagesChanged.Broadcast();
	};

	FPin* TargetPin = TargetNode->FindInput(PinName);
	if (!TargetPin)
	{
		// Array pin that was removed
		return nullptr;
	}

	if (TargetPin->Type.IsWildcard())
	{
		VOXEL_MESSAGE(Error, "Wildcard pin {0} needs to be converted. Please connect it to another pin or right click it -> Convert", TargetPin);
		return nullptr;
	}

	FNode& RootNode = GraphCompiler->NewNode(ENodeType::Root, TargetNode->NodeRef);
	TargetPin->CopyInputPinTo(RootNode.NewInputPin(TargetPin->Name, TargetPin->Type));
	GraphCompiler->RemoveNode(*TargetNode);

	GraphCompiler->DisconnectVirtualPins();
	GraphCompiler->RemoveNodesNotLinkedToRoot();

	if (Scope.HasError())
	{
		return nullptr;
	}

	GraphCompiler->Check();

	if (Scope.HasError())
	{
		return nullptr;
	}

	// Apply errors & exit if we have any
	for (const FNode& Node : GraphCompiler->GetNodes())
	{
		Node.ApplyErrors();
	}

	if (Scope.HasError())
	{
		return nullptr;
	}

	FNode& RuntimeRootNode = GraphCompiler->NewNode(RootNode.NodeRef);
	{
		FPin& InputPin = RootNode.GetInputPin(0);
		{
			const TSharedRef<FVoxelNode_Root> RootVoxelNode = MakeVoxelShared<FVoxelNode_Root>();
			RootVoxelNode->CreateInputPin(InputPin.Type, InputPin.Name);
			RuntimeRootNode.SetVoxelNode(RootVoxelNode);
		}

		InputPin.CopyInputPinTo(RuntimeRootNode.NewInputPin(InputPin.Name, InputPin.Type));
		GraphCompiler->RemoveNode(RootNode);
	}

	for (const FNode& Node : GraphCompiler->GetNodes())
	{
		if (!ensure(Node.Type == ENodeType::Struct))
		{
			VOXEL_MESSAGE(Error, "INTERNAL ERROR: Unexpected node {0}", Node);
			return nullptr;
		}
	}

	TMap<const FNode*, TSharedPtr<FVoxelNode>> Nodes;
	{
		VOXEL_SCOPE_COUNTER("Copy nodes");
		for (FNode& Node : GraphCompiler->GetNodes())
		{
			Nodes.Add(&Node, Node.GetVoxelNode().MakeSharedCopy());
		}
	}

	for (const auto& It : Nodes)
	{
		It.Value->InitializeNodeRuntime(It.Key->NodeRef, false);

		if (Scope.HasError())
		{
			return nullptr;
		}
	}

	for (const auto& It : Nodes)
	{
		const FNode& Node = *It.Key;
		FVoxelNode& VoxelNode = *It.Value;

		const TVoxelArray<TSharedPtr<FVoxelPin>> Pins = VoxelNode.GetNameToPin().ValueArray();
		ensure(Pins.Num() == Node.GetPins().Num());

		// Assign compute to input pins
		for (const TSharedPtr<FVoxelPin>& Pin : Pins)
		{
			if (!Pin->bIsInput ||
				Pin->Metadata.bVirtualPin)
			{
				continue;
			}

			const FPin& InputPin = Node.FindInputChecked(Pin->Name);
			if (!ensure(InputPin.Type == Pin->GetType()))
			{
				VOXEL_MESSAGE(Error, "{0}: Internal error", InputPin);
				return nullptr;
			}

			const FVoxelNodeRuntime::FPinData* OutputPinData = nullptr;
			if (InputPin.GetLinkedTo().Num() > 0)
			{
				check(InputPin.GetLinkedTo().Num() == 1);
				const FPin& OutputPin = InputPin.GetLinkedTo()[0];
				check(OutputPin.Direction == EPinDirection::Output);

				OutputPinData = &Nodes[&OutputPin.Node]->GetNodeRuntime().GetPinData(OutputPin.Name);
				check(!OutputPinData->bIsInput);
			}

			FVoxelNodeRuntime::FPinData& PinData = ConstCast(VoxelNode.GetNodeRuntime().GetPinData(Pin->Name));
			if (!ensure(PinData.Type == InputPin.Type))
			{
				VOXEL_MESSAGE(Error, "{0}: Internal error", InputPin);
				return nullptr;
			}

			if (OutputPinData)
			{
				PinData.Compute = OutputPinData->Compute;
			}
			else if (InputPin.Type.HasPinDefaultValue())
			{
				const FVoxelRuntimePinValue DefaultValue = FVoxelPinType::MakeRuntimeValueFromInnerValue(
					InputPin.Type,
					InputPin.GetDefaultValue());

				if (!ensureVoxelSlow(DefaultValue.IsValid()))
				{
					VOXEL_MESSAGE(Error, "{0}: Invalid default value", InputPin);
					return nullptr;
				}

				ensure(DefaultValue.GetType().CanBeCastedTo(InputPin.Type));

				if (VOXEL_DEBUG)
				{
					const FString DefaultValueString = InputPin.GetDefaultValue().ExportToString();

					FVoxelPinValue TestValue(InputPin.GetDefaultValue().GetType());
					ensure(TestValue.ImportFromString(DefaultValueString));
					ensure(TestValue == InputPin.GetDefaultValue());
				}
				if (VOXEL_DEBUG)
				{
					const FVoxelPinValue ExposedValue = FVoxelPinType::MakeExposedInnerValue(DefaultValue);
					ensure(InputPin.GetDefaultValue().GetType().CanBeCastedTo(ExposedValue.GetType()));

					// Types will be different when working with enums
					ensureVoxelSlow(
						ExposedValue.GetType() != InputPin.GetDefaultValue().GetType() ||
						ExposedValue == InputPin.GetDefaultValue());
				}

				PinData.Compute = MakeVoxelShared<FVoxelComputeValue>([Value = FVoxelFutureValue(DefaultValue)](const FVoxelQuery&)
				{
					return Value;
				});
			}
			else
			{
				FVoxelRuntimePinValue Value(InputPin.Type);
				if (InputPin.Type.IsBufferArray())
				{
					// We don't want to have the default element
					ConstCast(Value.Get<FVoxelBuffer>()).SetAsEmpty();
				}

				PinData.Compute = MakeVoxelShared<FVoxelComputeValue>([FutureValue = FVoxelFutureValue(Value)](const FVoxelQuery&)
				{
					return FutureValue;
				});
			}
		}
	}

	for (const auto& It : Nodes)
	{
		It.Value->RemoveEditorData();
		It.Value->EnableSharedNode(It.Value.ToSharedRef());
	}

	TVoxelSet<TSharedPtr<const FVoxelNode>> EvaluatorNodes;
	for (const auto& It : Nodes)
	{
		EvaluatorNodes.Add(It.Value);
	}

	const TSharedRef<FVoxelGraphEvaluator> Evaluator = MakeVoxelShared<FVoxelGraphEvaluator>();
	Evaluator->Initialize(
		PinName,
		CastChecked<FVoxelNode_Root>(Nodes[&RuntimeRootNode].ToSharedRef()),
		MoveTemp(EvaluatorNodes),
		GraphCompiler);
	return Evaluator;
}

TSharedPtr<const Voxel::Graph::FGraph> UVoxelTerminalGraphRuntime::Compile(
	const FOnVoxelGraphChanged& OnTranslated,
	const UVoxelTerminalGraph& TerminalGraph,
	const bool bEnableLogging,
	TVoxelArray<TSharedRef<FVoxelMessage>>& OutCompileMessages)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	using namespace Voxel::Graph;

	const FVoxelGraphCompileScope Scope(TerminalGraph, bEnableLogging);
	ON_SCOPE_EXIT
	{
		OutCompileMessages = Scope.GetMessages();
	};

	const TSharedRef<FVoxelGraphCompiler> GraphCompiler = MakeVoxelShared<FVoxelGraphCompiler>(TerminalGraph);
	if (!GraphCompiler->LoadSerializedGraph(OnTranslated))
	{
		return nullptr;
	}
	ensure(!Scope.HasError());

#define RUN_PASS(Name, ...) \
	ensure(!Scope.HasError()); \
	\
	GraphCompiler->Name(); \
	\
	if (Scope.HasError()) \
	{ \
		return nullptr;  \
	} \
	\
	GraphCompiler->Check(); \
	\
	if (Scope.HasError()) \
	{ \
		return nullptr;  \
	}

	RUN_PASS(AddPreviewNode);
	RUN_PASS(AddDebugNodes);
	RUN_PASS(AddRangeNodes);
	RUN_PASS(RemoveSplitPins);
	RUN_PASS(AddWildcardErrors);
	RUN_PASS(AddNoDefaultErrors);
	RUN_PASS(CheckParameters);
	RUN_PASS(CheckInputs);
	RUN_PASS(CheckOutputs);
	RUN_PASS(AddToBuffer);
	RUN_PASS(RemoveLocalVariables);
	RUN_PASS(CollapseInputs);
	RUN_PASS(ReplaceTemplates);
	RUN_PASS(RemovePassthroughs);
	RUN_PASS(RemoveNodesNotLinkedToQueryableNodes);
	RUN_PASS(CheckForLoops);

#undef RUN_PASS

	// Do this at the end, will assert before CheckOutputs if we have multiple outputs
	{
		TVoxelSet<FVoxelGraphNodeRef> VisitedRefs;
		VisitedRefs.Reserve(GraphCompiler->GetNodes().Num());

		for (const FNode& Node : GraphCompiler->GetNodes())
		{
			if (VisitedRefs.Contains(Node.NodeRef))
			{
				ensure(false);
				VOXEL_MESSAGE(Error, "Duplicated node ref {0}", Node.NodeRef);
				continue;
			}

			VisitedRefs.Add(Node.NodeRef);
		}
	}

	if (Scope.HasError())
	{
		return nullptr;
	}

	// Apply errors but don't exit on error - some pin refs might still compile successfully
	for (const FNode& Node : GraphCompiler->GetNodes())
	{
		Node.ApplyErrors();
	}

	return GraphCompiler->Clone();
}