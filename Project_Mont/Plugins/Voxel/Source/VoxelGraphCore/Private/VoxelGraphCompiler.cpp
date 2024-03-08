// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphCompiler.h"
#include "VoxelGraph.h"
#include "VoxelExecNode.h"
#include "VoxelTemplateNode.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphCompileScope.h"
#include "Preview/VoxelNode_Preview.h"
#include "Nodes/VoxelNode_Debug.h"
#include "Nodes/VoxelNode_Output.h"
#include "Nodes/VoxelNode_UFunction.h"
#include "Nodes/VoxelNode_Parameter.h"
#include "Nodes/VoxelNode_RangeDebug.h"
#include "Nodes/VoxelInputNodes.h"
#include "Nodes/VoxelLocalVariableNodes.h"
#include "Nodes/VoxelExecNode_CallTerminalGraph.h"
#include "FunctionLibrary/VoxelBasicFunctionLibrary.h"

FVoxelGraphCompiler::FVoxelGraphCompiler(const UVoxelTerminalGraph& TerminalGraph)
	: TerminalGraph(TerminalGraph)
	, SerializedGraph(TerminalGraph.GetRuntime().GetSerializedGraph())
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelGraphCompiler::LoadSerializedGraph(const FOnVoxelGraphChanged& OnTranslated)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	check(GVoxelGraphCompileScope);

	// Manually call FixupPins on graph calls
	// This needs to happen as late as possible as it requires graphs to be loaded
	{
		VOXEL_SCOPE_COUNTER("Fixup graph calls");

		for (const auto& It : SerializedGraph.NodeNameToNode)
		{
			if (!It.Value.VoxelNode)
			{
				continue;
			}

			const FVoxelExecNode_CallTerminalGraph* CallTerminalGraphNode = It.Value.VoxelNode->As<FVoxelExecNode_CallTerminalGraph>();
			if (!CallTerminalGraphNode)
			{
				continue;
			}

			ConstCast(CallTerminalGraphNode)->FixupPins(TerminalGraph.GetGraph(), OnTranslated);
		}
	}

	TVoxelMap<const FVoxelSerializedNode*, FNode*> SerializedNodeToNode;
	TVoxelSet<const FVoxelSerializedNode*> SkippedNodes;

	for (const auto& NodeIt : SerializedGraph.NodeNameToNode)
	{
		const FVoxelSerializedNode& SerializedNode = NodeIt.Value;
		VOXEL_SCOPE_COUNTER_FNAME(SerializedNode.EdGraphNodeTitle);

		if (!SerializedNode.VoxelNode.IsValid())
		{
			for (const auto& It : SerializedNode.InputPins)
			{
				if (It.Value.LinkedTo.Num() > 0)
				{
					VOXEL_MESSAGE(Error, "Invalid struct on {0}", SerializedNode);
					return false;
				}
			}
			for (const auto& It : SerializedNode.OutputPins)
			{
				if (It.Value.LinkedTo.Num() > 0)
				{
					VOXEL_MESSAGE(Error, "Invalid struct on {0}", SerializedNode);
					return false;
				}
			}

			// This node isn't connected to anything, just skip it
			VOXEL_MESSAGE(Warning, "Invalid struct on {0}", SerializedNode);
			SkippedNodes.Add(&SerializedNode);
			continue;
		}

		const FVoxelGraphNodeRef NodeRef
		{
			FVoxelTerminalGraphRef(TerminalGraph),
			SerializedNode.GetNodeId(),
			SerializedNode.EdGraphNodeTitle,
			SerializedNode.EdGraphNodeName
		};

		FNode& Node = NewNode(NodeRef);
		{
			VOXEL_SCOPE_COUNTER_FORMAT("Copy node %s", *SerializedNode.VoxelNode->GetStruct()->GetName());
			Node.SetVoxelNode(SerializedNode.VoxelNode->MakeSharedCopy());
		}
		SerializedNodeToNode.Add_EnsureNew(&SerializedNode, &Node);

		for (const FString& Warning : SerializedNode.Warnings)
		{
			Node.AddError(Warning);
		}
		for (const FString& Error : SerializedNode.Errors)
		{
			Node.AddError(Error);
		}

		const FVoxelNode& VoxelNode = Node.GetVoxelNode();
		for (const FVoxelPin& Pin : VoxelNode.GetPins())
		{
			if (!(Pin.bIsInput ? SerializedNode.InputPins : SerializedNode.OutputPins).Contains(Pin.Name))
			{
				VOXEL_MESSAGE(Error, "Outdated node {0}: missing pin {1}", SerializedNode, Pin.Name);
				return false;
			}
		}
		for (const auto& It : SerializedNode.InputPins)
		{
			const FVoxelSerializedPin& SerializedPin = It.Value;
			if (!SerializedPin.ParentPinName.IsNone())
			{
				// Sub-pinS
				continue;
			}

			const TSharedPtr<const FVoxelPin> Pin = VoxelNode.FindPin(SerializedPin.PinName);
			if (!Pin ||
				!Pin->bIsInput)
			{
				VOXEL_MESSAGE(Error, "Outdated node {0}: unknown pin {1}", SerializedNode, SerializedPin.PinName);
				return false;
			}

			if (SerializedPin.Type != Pin->GetType())
			{
				VOXEL_MESSAGE(Error, "Outdated node {0}: type mismatch for pin {1}", SerializedNode, SerializedPin.PinName);
				return false;
			}
		}
		for (const auto& It : SerializedNode.OutputPins)
		{
			const FVoxelSerializedPin& SerializedPin = It.Value;
			if (!SerializedPin.ParentPinName.IsNone())
			{
				// Sub-pinS
				continue;
			}

			const TSharedPtr<const FVoxelPin> Pin = VoxelNode.FindPin(SerializedPin.PinName);
			if (!Pin ||
				Pin->bIsInput)
			{
				VOXEL_MESSAGE(Error, "Outdated node {0}: unknown pin {1}", SerializedNode, SerializedPin.PinName);
				return false;
			}

			if (SerializedPin.Type != Pin->GetType())
			{
				VOXEL_MESSAGE(Error, "Outdated node {0}: type mismatch for pin {1}", SerializedNode, SerializedPin.PinName);
				return false;
			}
		}

		for (const auto& It : SerializedNode.InputPins)
		{
			const FVoxelSerializedPin& SerializedPin = It.Value;
			if (!SerializedPin.Type.IsValid())
			{
				VOXEL_MESSAGE(Error, "Invalid pin {0}.{1}", SerializedNode, SerializedPin.PinName);
				return false;
			}

			FVoxelPinValue DefaultValue;
			if (SerializedPin.Type.HasPinDefaultValue())
			{
				DefaultValue = SerializedPin.DefaultValue;

				if (!DefaultValue.IsValid() ||
					!ensureVoxelSlow(DefaultValue.GetType().CanBeCastedTo(SerializedPin.Type.GetPinDefaultValueType())))
				{
					VOXEL_MESSAGE(Error, "{0}.{1}: Invalid default value", SerializedNode, SerializedPin.PinName);
					return false;
				}
			}
			else
			{
				ensureVoxelSlow(!SerializedPin.DefaultValue.IsValid());
			}

			Node.NewInputPin(SerializedPin.PinName, SerializedPin.Type, DefaultValue).SetParentName(SerializedPin.ParentPinName);
		}

		for (const auto& It : SerializedNode.OutputPins)
		{
			const FVoxelSerializedPin& SerializedPin = It.Value;
			if (!SerializedPin.Type.IsValid())
			{
				VOXEL_MESSAGE(Error, "Invalid pin {0}.{1}", SerializedNode, SerializedPin.PinName);
				return false;
			}

			Node.NewOutputPin(SerializedPin.PinName, SerializedPin.Type).SetParentName(SerializedPin.ParentPinName);
		}
	}

	VOXEL_SCOPE_COUNTER("Fixup links");

	// Fixup links after all nodes are created
	for (const auto& SerializedNodeIt : SerializedGraph.NodeNameToNode)
	{
		const FVoxelSerializedNode& SerializedNode = SerializedNodeIt.Value;
		if (SkippedNodes.Contains(&SerializedNode))
		{
			continue;
		}

		FNode& Node = *SerializedNodeToNode[&SerializedNode];

		for (const auto& It : SerializedNode.InputPins)
		{
			const FVoxelSerializedPin& InputPin = It.Value;
			if (InputPin.LinkedTo.Num() > 1)
			{
				VOXEL_MESSAGE(Error, "Too many pins linked to {0}.{1}", SerializedNode, InputPin.PinName);
				return false;
			}

			for (const FVoxelSerializedPinRef& OutputPinRef : InputPin.LinkedTo)
			{
				check(!OutputPinRef.bIsInput);

				const FVoxelSerializedPin* OutputPin = SerializedGraph.FindPin(OutputPinRef);
				if (!ensure(OutputPin))
				{
					VOXEL_MESSAGE(Error, "Invalid pin ref on {0}", SerializedNode);
					continue;
				}

				const FVoxelSerializedNode& OtherSerializedNode = SerializedGraph.NodeNameToNode[OutputPinRef.NodeName];
				FNode& OtherNode = *SerializedNodeToNode[&OtherSerializedNode];

				if (!OutputPin->Type.CanBeCastedTo_Schema(InputPin.Type))
				{
					VOXEL_MESSAGE(Error, "Invalid pin link from {0}.{1} to {2}.{3}: type mismatch: {4} vs {5}",
						SerializedNode,
						OutputPin->PinName,
						OtherSerializedNode,
						InputPin.PinName,
						OutputPin->Type.ToString(),
						InputPin.Type.ToString());

					continue;
				}

				Node.FindInputChecked(InputPin.PinName).MakeLinkTo(OtherNode.FindOutputChecked(OutputPinRef.PinName));
			}
		}
	}

	// Input links are used to populate all links, check they're correct with output links
	for (const auto& SerializedNodeIt : SerializedGraph.NodeNameToNode)
	{
		const FVoxelSerializedNode& SerializedNode = SerializedNodeIt.Value;
		if (SkippedNodes.Contains(&SerializedNode))
		{
			continue;
		}

		FNode& Node = *SerializedNodeToNode[&SerializedNode];

		for (const auto& It : SerializedNode.OutputPins)
		{
			const FVoxelSerializedPin& OutputPin = It.Value;
			for (const FVoxelSerializedPinRef& InputPinRef : OutputPin.LinkedTo)
			{
				check(InputPinRef.bIsInput);

				const FVoxelSerializedPin* InputPin = SerializedGraph.FindPin(InputPinRef);
				if (!ensure(InputPin))
				{
					VOXEL_MESSAGE(Error, "Invalid pin ref on {0}", SerializedNode);
					continue;
				}

				const FVoxelSerializedNode& OtherSerializedNode = SerializedGraph.NodeNameToNode[InputPinRef.NodeName];
				FNode& OtherNode = *SerializedNodeToNode[&OtherSerializedNode];

				if (!OutputPin.Type.CanBeCastedTo_Schema(InputPin->Type))
				{
					VOXEL_MESSAGE(Error, "Invalid pin link from {0}.{1} to {2}.{3}: type mismatch: {4} vs {5}",
						SerializedNode,
						OutputPin.PinName,
						OtherSerializedNode,
						InputPin->PinName,
						OutputPin.Type.ToString(),
						InputPin->Type.ToString());

					return false;
				}

				if (!ensure(Node.FindOutputChecked(OutputPin.PinName).IsLinkedTo(OtherNode.FindInputChecked(InputPinRef.PinName))))
				{
					VOXEL_MESSAGE(Error, "Translation error: {0} -> {1}", Node, OtherNode);
					return false;
				}
			}
		}
	}

	Check();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::AddPreviewNode()
{
	VOXEL_FUNCTION_COUNTER();

	// Make sure to run AddPreviewNode before any new node is spawned, otherwise the find below will fail
	// and find compile nodes instead

	const FVoxelSerializedPinRef PreviewedSerializedPin = SerializedGraph.PreviewedPin;
	if (PreviewedSerializedPin.NodeName.IsNone())
	{
		ensure(PreviewedSerializedPin.PinName.IsNone());
		ensure(!PreviewedSerializedPin.bIsInput);
		return;
	}

	FPin* PreviewedPin = nullptr;
	for (FNode& Node : GetNodes())
	{
		if (Node.NodeRef.EdGraphNodeName != PreviewedSerializedPin.NodeName)
		{
			continue;
		}

		ensure(!PreviewedPin);
		PreviewedPin = Node.FindPin(PreviewedSerializedPin.PinName);
		ensure(PreviewedPin);
	}

	if (!PreviewedPin)
	{
		VOXEL_MESSAGE(Warning, "Invalid preview pin");
		return;
	}

	const FVoxelGraphNodeRef NodeRef
	{
		FVoxelTerminalGraphRef(TerminalGraph),
		FVoxelGraphConstants::NodeId_Preview,
		"Preview Node",
		{}
	};

	FNode& PreviewNode = NewNode(NodeRef);

	const TSharedRef<FVoxelNode_Preview> PreviewVoxelNode = MakeVoxelShared<FVoxelNode_Preview>();
	PreviewVoxelNode->PromotePin_Runtime(PreviewVoxelNode->GetUniqueInputPin(), PreviewedPin->Type);

	PreviewNode.SetVoxelNode(PreviewVoxelNode);
	PreviewNode.NewInputPin(VOXEL_PIN_NAME(FVoxelNode_Preview, ValuePin), PreviewedPin->Type);

	if (PreviewedPin->Direction == EPinDirection::Input)
	{
		// TODO Is previewing an input allowed?
		PreviewedPin->CopyInputPinTo(PreviewNode.GetInputPin(0));
	}
	else
	{
		check(PreviewedPin->Direction == EPinDirection::Output);
		PreviewedPin->MakeLinkTo(PreviewNode.GetInputPin(0));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::AddDebugNodes()
{
	VOXEL_FUNCTION_COUNTER();

	for (const FName NodeName : SerializedGraph.DebuggedNodes)
	{
		for (FNode& Node : GetNodes())
		{
			if (Node.NodeRef.EdGraphNodeName != NodeName)
			{
				continue;
			}

			for (FPin& Pin : Node.GetOutputPins())
			{
				FNode& DebugNode = NewNode(Node.NodeRef.WithSuffix("DebugNode").WithSuffix(Pin.Name.ToString()));

				const TSharedRef<FVoxelNode_Debug> DebugVoxelNode = MakeVoxelShared<FVoxelNode_Debug>();
				DebugVoxelNode->PromotePin_Runtime(DebugVoxelNode->GetUniqueInputPin(), Pin.Type);
				DebugVoxelNode->PromotePin_Runtime(DebugVoxelNode->GetUniqueOutputPin(), Pin.Type);

				DebugNode.SetVoxelNode(DebugVoxelNode);
				DebugNode.NewInputPin(VOXEL_PIN_NAME(FVoxelNode_Debug, InPin), Pin.Type);
				DebugNode.NewOutputPin(VOXEL_PIN_NAME(FVoxelNode_Debug, OutPin), Pin.Type);

				Pin.CopyOutputPinTo(DebugNode.GetOutputPin(0));
				Pin.BreakAllLinks();

				Pin.MakeLinkTo(DebugNode.GetInputPin(0));
			}

			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::AddRangeNodes()
{
#if WITH_EDITOR
	if (!TerminalGraph.GetGraph().bEnableNodeRangeStats)
	{
		return;
	}

	VOXEL_FUNCTION_COUNTER();

	for (FNode& Node : GetNodesCopy())
	{
		for (FPin& Pin : Node.GetOutputPins())
		{
			if (Pin.Type.IsWildcard())
			{
				continue;
			}

			FNode& DebugNode = NewNode(Node.NodeRef.WithSuffix("DebugRangeNode").WithSuffix(Pin.Name.ToString()));

			const TSharedRef<FVoxelNode_RangeDebug> DebugVoxelNode = MakeVoxelShared<FVoxelNode_RangeDebug>();
			DebugVoxelNode->PromotePin_Runtime(DebugVoxelNode->GetUniqueInputPin(), Pin.Type);
			DebugVoxelNode->PromotePin_Runtime(DebugVoxelNode->GetUniqueOutputPin(), Pin.Type);
			DebugVoxelNode->RefPin = Pin.Name;

			DebugNode.SetVoxelNode(DebugVoxelNode);
			DebugNode.NewInputPin(VOXEL_PIN_NAME(FVoxelNode_RangeDebug, InPin), Pin.Type);
			DebugNode.NewOutputPin(VOXEL_PIN_NAME(FVoxelNode_RangeDebug, OutPin), Pin.Type);

			Pin.CopyOutputPinTo(DebugNode.GetOutputPin(0));
			Pin.BreakAllLinks();

			Pin.MakeLinkTo(DebugNode.GetInputPin(0));
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::RemoveSplitPins()
{
	VOXEL_FUNCTION_COUNTER();

	for (FNode& Node : GetNodesCopy())
	{
		TVoxelMap<FPin*, FNode*> ParentPinToMakeBreakNodes;
		TVoxelArray<FPin*> PinsToRemove;
		for (FPin& SubPin : Node.GetPins())
		{
			if (SubPin.GetParentName().IsNone())
			{
				continue;
			}

			FPin* ParentPin = Node.FindPin(SubPin.GetParentName());
			if (!ensure(ParentPin))
			{
				VOXEL_MESSAGE(Error, "{0}: Invalid parent pin", SubPin);
				return;
			}

			FNode*& MakeBreakNode = ParentPinToMakeBreakNodes.FindOrAdd(ParentPin);
			if (!MakeBreakNode)
			{
				if (ParentPin->Direction == EPinDirection::Input)
				{
					FNode& MakeNode = NewNode(Node.NodeRef.WithSuffix("Make_" + ParentPin->Name.ToString()));
					MakeBreakNode = &MakeNode;

					const FVoxelNode* MakeVoxelNodeTemplate = SerializedGraph.FindMakeNode(ParentPin->Type);
					if (!ensure(MakeVoxelNodeTemplate))
					{
						VOXEL_MESSAGE(Error, "{0}: No make node for type {1}", ParentPin, ParentPin->Type.ToString());
						return;
					}

					const TSharedRef<FVoxelNode> MakeVoxelNode = MakeVoxelNodeTemplate->MakeSharedCopy();
					MakeVoxelNode->PromotePin_Runtime(MakeVoxelNode->GetUniqueOutputPin(), ParentPin->Type);
					MakeNode.SetVoxelNode(MakeVoxelNode);

					MakeNode.NewOutputPin(
						MakeVoxelNode->GetUniqueOutputPin().Name,
						MakeVoxelNode->GetUniqueOutputPin().GetType()).MakeLinkTo(*ParentPin);

					for (const FVoxelPin& Pin : MakeVoxelNode->GetPins())
					{
						if (!Pin.bIsInput)
						{
							continue;
						}

						MakeNode.NewInputPin(Pin.Name, Pin.GetType());
					}
				}
				else
				{
					check(ParentPin->Direction == EPinDirection::Output);

					FNode& BreakNode = NewNode(Node.NodeRef.WithSuffix("Break_" + ParentPin->Name.ToString()));
					MakeBreakNode = &BreakNode;

					const FVoxelNode* BreakVoxelNodeTemplate = SerializedGraph.FindBreakNode(ParentPin->Type);
					if (!ensure(BreakVoxelNodeTemplate))
					{
						VOXEL_MESSAGE(Error, "{0}: No break node for type {1}", ParentPin, ParentPin->Type.ToString());
						return;
					}

					const TSharedRef<FVoxelNode> BreakVoxelNode = BreakVoxelNodeTemplate->MakeSharedCopy();
					BreakVoxelNode->PromotePin_Runtime(BreakVoxelNode->GetUniqueInputPin(), ParentPin->Type);
					BreakNode.SetVoxelNode(BreakVoxelNode);

					BreakNode.NewInputPin(
						BreakVoxelNode->GetUniqueInputPin().Name,
						BreakVoxelNode->GetUniqueInputPin().GetType()).MakeLinkTo(*ParentPin);

					for (const FVoxelPin& Pin : BreakVoxelNode->GetPins())
					{
						if (Pin.bIsInput)
						{
							continue;
						}

						BreakNode.NewOutputPin(Pin.Name, Pin.GetType());
					}
				}
			}
			check(MakeBreakNode);

			TArray<FString> Parts;
			SubPin.Name.ToString().ParseIntoArray(Parts, TEXT("|"));

			FName SubPinName = SubPin.Name;
			if (Parts.Num() > 1)
			{
				SubPinName = FName(Parts.Last());
			}

			FPin* NewPin = MakeBreakNode->FindPin(SubPinName);
			if (!ensure(NewPin))
			{
				VOXEL_MESSAGE(Error, "{0}: Invalid sub-pin", SubPin);
				return;
			}
			ensure(NewPin->GetLinkedTo().Num() == 0);

			if (SubPin.Direction == EPinDirection::Input)
			{
				SubPin.CopyInputPinTo(*NewPin);
			}
			else
			{
				check(SubPin.Direction == EPinDirection::Output);
				SubPin.CopyOutputPinTo(*NewPin);
			}

			PinsToRemove.Add(&SubPin);
		}

		for (FPin* Pin : PinsToRemove)
		{
			Node.RemovePin(*Pin);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::AddWildcardErrors()
{
	VOXEL_FUNCTION_COUNTER();

	for (FNode& Node : GetNodes())
	{
		for (const FPin& Pin : Node.GetPins())
		{
			if (Pin.Type.IsWildcard())
			{
				Node.AddError("Wildcard pin " + Pin.Name.ToString() + " needs to be converted. Please connect it to another pin or right click it -> Convert");
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::AddNoDefaultErrors()
{
	VOXEL_FUNCTION_COUNTER();

	for (FNode& Node : GetNodes())
	{
		for (const FPin& Pin : Node.GetPins())
		{
			if (Pin.GetLinkedTo().Num() > 0)
			{
				continue;
			}

			const TSharedPtr<const FVoxelPin> VoxelPin = Node.GetVoxelNode().FindPin(Pin.Name);
			if (!ensure(VoxelPin) ||
				!VoxelPin->Metadata.bNoDefault)
			{
				continue;
			}

			Node.AddError("Pin " + Pin.Name.ToString() + " needs to be connected");
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::CheckParameters() const
{
	VOXEL_FUNCTION_COUNTER();

	for (const FNode& Node : GetNodes())
	{
		const FVoxelNode_Parameter* ParameterNode = Node.GetVoxelNode().As<FVoxelNode_Parameter>();
		if (!ParameterNode)
		{
			continue;
		}

		if (!TerminalGraph.GetGraph().FindParameter(ParameterNode->ParameterGuid))
		{
			VOXEL_MESSAGE(Error, "{0}: No parameter named {1}", Node, ParameterNode->ParameterName);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::CheckInputs() const
{
	VOXEL_FUNCTION_COUNTER();

	const auto GetTerminalGraph = [&](const bool bIsGraphInput) -> const UVoxelTerminalGraph&
	{
		return bIsGraphInput ? TerminalGraph.GetGraph().GetMainTerminalGraph() : TerminalGraph;
	};

	const auto GetInputName = [&](
		const FGuid& Guid,
		const bool bIsGraphInput)
	{
		const FVoxelGraphInput* Input = GetTerminalGraph(bIsGraphInput).FindInput(Guid);
		if (!Input)
		{
			return Guid.ToString();
		}

		return Input->Name.ToString();
	};

	TVoxelMap<TPair<FGuid, bool>, TArray<const FNode*>> InputGuidToDefaultNodes;
	for (const FNode& Node : GetNodes())
	{
		if (const FVoxelNode_Input_WithDefaultPin* InputNode = Node.GetVoxelNode().As<FVoxelNode_Input_WithDefaultPin>())
		{
			if (!GetTerminalGraph(InputNode->bIsGraphInput).FindInput(InputNode->Guid))
			{
				VOXEL_MESSAGE(Error, "{0}: No input with GUID {1}",
					Node,
					GetInputName(InputNode->Guid, InputNode->bIsGraphInput));
			}

			InputGuidToDefaultNodes.FindOrAdd({ InputNode->Guid, InputNode->bIsGraphInput }).Add(&Node);
		}

		if (const FVoxelNode_Input_WithoutDefaultPin* InputNode = Node.GetVoxelNode().As<FVoxelNode_Input_WithoutDefaultPin>())
		{
			if (!GetTerminalGraph(InputNode->bIsGraphInput).FindInput(InputNode->Guid))
			{
				VOXEL_MESSAGE(Error, "{0}: No input with GUID {1}",
					Node,
					GetInputName(InputNode->Guid, InputNode->bIsGraphInput));
			}
		}
	}

	for (const auto& It : InputGuidToDefaultNodes)
	{
		if (It.Value.Num() > 1)
		{
			VOXEL_MESSAGE(Error, "Multiple input nodes with a default pin for input {0}: {1}",
				GetInputName(It.Key.Key, It.Key.Value),
				It.Value);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::CheckOutputs() const
{
	VOXEL_FUNCTION_COUNTER();

	const auto GetOutputName = [&](const FGuid& Guid)
	{
		const FVoxelGraphOutput* Output = TerminalGraph.FindOutput(Guid);
		if (!Output)
		{
			return Guid.ToString();
		}

		return Output->Name.ToString();
	};

	TVoxelMap<FGuid, const FNode*> OutputGuidToNode;
	for (const FNode& Node : GetNodes())
	{
		const FVoxelNode_Output* OutputNode = Node.GetVoxelNode().As<FVoxelNode_Output>();
		if (!OutputNode)
		{
			continue;
		}

		if (!TerminalGraph.FindOutput(OutputNode->Guid))
		{
			VOXEL_MESSAGE(Error, "{0}: No output named {1}", Node, GetOutputName(OutputNode->Guid));
			continue;
		}

		const FNode*& RootNode = OutputGuidToNode.FindOrAdd(OutputNode->Guid);
		if (RootNode)
		{
			VOXEL_MESSAGE(Error, "Duplicated output nodes: {0}, {1}", RootNode, Node);
			continue;
		}

		RootNode = &Node;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::AddToBuffer()
{
	VOXEL_FUNCTION_COUNTER();

	for (FNode& Node : GetNodesCopy())
	{
		for (FPin& ToPin : Node.GetInputPins())
		{
			if (!ToPin.Type.IsBuffer())
			{
				continue;
			}
			FNode* ToBufferNode = nullptr;

			for (FPin& FromPin : ToPin.GetLinkedTo())
			{
				if (FromPin.Type.IsBuffer())
				{
					continue;
				}

				if (!ToBufferNode)
				{
					ToBufferNode = &NewNode(Node.NodeRef.WithSuffix(ToPin.Name.ToString() + "_ToBuffer"));

					const TSharedRef<FVoxelNode_UFunction> FunctionNode = FVoxelNode_UFunction::Make(FindUFunctionChecked(UVoxelBasicFunctionLibrary, ToBuffer));
					FunctionNode->PromotePin_Runtime(FunctionNode->GetUniqueInputPin(), FromPin.Type);
					FunctionNode->PromotePin_Runtime(FunctionNode->GetUniqueOutputPin(), ToPin.Type);
					ToBufferNode->SetVoxelNode(FunctionNode);

					ToBufferNode->NewInputPin(FunctionNode->GetUniqueInputPin().Name, FromPin.Type);
					ToBufferNode->NewOutputPin(FunctionNode->GetUniqueOutputPin().Name, ToPin.Type);
				}

				FromPin.MakeLinkTo(ToBufferNode->GetInputPin(0));
				ToPin.MakeLinkTo(ToBufferNode->GetOutputPin(0));
				ToPin.BreakLinkTo(FromPin);
			}
		}
	}

	for (const FNode& Node : GetNodes())
	{
		for (const FPin& ToPin : Node.GetInputPins())
		{
			for (const FPin& FromPin : ToPin.GetLinkedTo())
			{
				ensure(FromPin.Type.CanBeCastedTo(ToPin.Type));
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::RemoveLocalVariables()
{
	VOXEL_FUNCTION_COUNTER();

	TMap<FGuid, FNode*> GuidToDeclaration;
	for (FNode& Node : GetNodes())
	{
		const FVoxelNode_LocalVariableDeclaration* Declaration = Node.GetVoxelNode().As<FVoxelNode_LocalVariableDeclaration>();
		if (!Declaration)
		{
			continue;
		}

		if (!TerminalGraph.FindLocalVariable(Declaration->Guid))
		{
			VOXEL_MESSAGE(
				Error,
				"Invalid local variable node: {0}",
				Declaration);
			return;
		}

		if (const FNode* ExistingNode = GuidToDeclaration.FindRef(Declaration->Guid))
		{
			VOXEL_MESSAGE(
				Error,
				"Multiple local variable declarations not supported: {0}, {1}",
				ExistingNode,
				Node);
			return;
		}

		GuidToDeclaration.Add(Declaration->Guid, &Node);
	}

	TMap<FNode*, TArray<FNode*>> DeclarationToUsage;
	for (FNode& Node : GetNodes())
	{
		const FVoxelNode_LocalVariableUsage* Usage = Node.GetVoxelNode().As<FVoxelNode_LocalVariableUsage>();
		if (!Usage)
		{
			continue;
		}

		FNode* Declaration = GuidToDeclaration.FindRef(Usage->Guid);
		if (!Declaration)
		{
			VOXEL_MESSAGE(
				Error,
				"Missing local variable declaration: {0}",
				Node);
			return;
		}

		DeclarationToUsage.FindOrAdd(Declaration).Add(&Node);
	}

	// Remove usages
	for (auto& It : DeclarationToUsage)
	{
		FNode& Declaration = *It.Key;

		const FPin& InputPin = Declaration.GetInputPin(0);
		for (FNode* Usage : It.Value)
		{
			for (FPin& LinkedTo : Usage->GetOutputPin(0).GetLinkedTo())
			{
				InputPin.CopyInputPinTo(LinkedTo);
			}

			RemoveNode(*Usage);
		}
	}

	// Remove declarations
	for (const auto& It : GuidToDeclaration)
	{
		FNode& Declaration = *It.Value;

		const FPin& InputPin = Declaration.GetInputPin(0);
		for (FPin& LinkedTo : Declaration.GetOutputPin(0).GetLinkedTo())
		{
			InputPin.CopyInputPinTo(LinkedTo);
		}
		RemoveNode(Declaration);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::CollapseInputs()
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelMap<FGuid, FNode*> InputGuidToNode;

	// Find which input node to use for each input name
	for (FNode& Node : GetNodes())
	{
		if (const FVoxelNode_Input_WithDefaultPin* InputNode = Node.GetVoxelNode().As<FVoxelNode_Input_WithDefaultPin>())
		{
			FNode*& RootNode = InputGuidToNode.FindOrAdd(InputNode->Guid);

			// Should be handled by CheckInputs
			ensure(
				!RootNode ||
				// Other input pins without a default pins are allowed
				RootNode->GetVoxelNode().IsA<FVoxelNode_Input_WithoutDefaultPin>());

			RootNode = &Node;
		}

		if (const FVoxelNode_Input_WithoutDefaultPin* InputNode = Node.GetVoxelNode().As<FVoxelNode_Input_WithoutDefaultPin>())
		{
			FNode*& RootNode = InputGuidToNode.FindOrAdd(InputNode->Guid);
			if (!RootNode)
			{
				RootNode = &Node;
			}
		}
	}

	// Move other nodes link to the root input nodes
	for (FNode& Node : GetNodesCopy())
	{
		// No need to check for WithDefault, it's always going to be the root node
		const FVoxelNode_Input_WithoutDefaultPin* InputNode = Node.GetVoxelNode().As<FVoxelNode_Input_WithoutDefaultPin>();
		if (!InputNode)
		{
			continue;
		}

		FNode* RootNode = InputGuidToNode.FindRef(InputNode->Guid);
		if (!ensure(RootNode) ||
			RootNode == &Node)
		{
			continue;
		}

		ensure(Node.GetInputPins().Num() == 0);
		ensure(Node.GetOutputPins().Num() == 1);

		Node.GetOutputPin(0).CopyOutputPinTo(RootNode->GetOutputPin(0));
		RemoveNode(Node);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::ReplaceTemplates()
{
	VOXEL_FUNCTION_COUNTER();

	while (true)
	{
		if (!ReplaceTemplatesImpl())
		{
			// No nodes removed, exit
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::RemovePassthroughs()
{
	VOXEL_FUNCTION_COUNTER();

	RemoveNodes([&](FNode& Node)
	{
		if (Node.Type != ENodeType::Passthrough)
		{
			return false;
		}

		const int32 Num = Node.GetInputPins().Num();
		check(Num == Node.GetOutputPins().Num());

		for (int32 Index = 0; Index < Num; Index++)
		{
			const FPin& InputPin = Node.GetInputPin(Index);
			const FPin& OutputPin = Node.GetOutputPin(Index);

			for (FPin& LinkedTo : OutputPin.GetLinkedTo())
			{
				InputPin.CopyInputPinTo(LinkedTo);
			}
		}

		return true;
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::RemoveNodesNotLinkedToQueryableNodes()
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<const FNode*> NodesToKeep;
	for (const FNode& Node : GetNodes())
	{
		if (Node.Type != ENodeType::Struct)
		{
			continue;
		}

		if (Node.GetVoxelNode().IsA<FVoxelExecNode>() ||
			// Output nodes can be queried to get their value
			Node.GetVoxelNode().IsA<FVoxelNode_Output>() ||
			// Preview nodes are queried by preview widget
			Node.GetVoxelNode().IsA<FVoxelNode_Preview>())
		{
			NodesToKeep.Add(&Node);
		}
	}
	RemoveNodesImpl(NodesToKeep);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::CheckForLoops()
{
	VOXEL_FUNCTION_COUNTER();

	TSet<FNode*> VisitedNodes;
	TVoxelArray<FNode*> NodesToSort = GetNodesArray();

	while (NodesToSort.Num() > 0)
	{
		bool bHasRemovedNode = false;
		for (int32 Index = 0; Index < NodesToSort.Num(); Index++)
		{
			FNode* Node = NodesToSort[Index];

			const bool bHasInputPin = INLINE_LAMBDA
			{
				for (const FPin& Pin : Node->GetInputPins())
				{
					for (const FPin& LinkedTo : Pin.GetLinkedTo())
					{
						if (!VisitedNodes.Contains(&LinkedTo.Node))
						{
							return true;
						}
					}
				}
				return false;
			};

			if (bHasInputPin)
			{
				continue;
			}

			VisitedNodes.Add(Node);

			NodesToSort.RemoveAtSwap(Index);
			Index--;
			bHasRemovedNode = true;
		}

		if (!bHasRemovedNode &&
			NodesToSort.Num() > 0)
		{
			VOXEL_MESSAGE(Error, "Loop detected: {0}", NodesToSort);
			return;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::DisconnectVirtualPins()
{
	VOXEL_FUNCTION_COUNTER();

	for (FNode& Node : GetNodes())
	{
		if (Node.Type != ENodeType::Struct)
		{
			continue;
		}

		for (FPin& Pin : Node.GetInputPins())
		{
			const TSharedPtr<const FVoxelPin> VoxelPin = Node.GetVoxelNode().FindPin(Pin.Name);
			if (!ensure(VoxelPin) ||
				!VoxelPin->Metadata.bVirtualPin)
			{
				continue;
			}
			ensure(Pin.Direction == EPinDirection::Input);

			Pin.BreakAllLinks();

			if (Pin.Type.HasPinDefaultValue())
			{
				Pin.SetDefaultValue(FVoxelPinValue(Pin.Type.GetPinDefaultValueType()));
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphCompiler::RemoveNodesNotLinkedToRoot()
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<const FNode*> NodesToKeep;
	for (const FNode& Node : GetNodes())
	{
		if (Node.Type == ENodeType::Root)
		{
			NodesToKeep.Add(&Node);
		}
	}
	ensure(NodesToKeep.Num() == 1);

	RemoveNodesImpl(NodesToKeep);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelGraphCompiler::ReplaceTemplatesImpl()
{
	VOXEL_FUNCTION_COUNTER();

	for (FNode& Node : GetNodes())
	{
		if (Node.Type != ENodeType::Struct ||
			!Node.GetVoxelNode().IsA<FVoxelTemplateNode>())
		{
			continue;
		}

		bool bHasWildcardPin = false;
		for (const FPin& Pin : Node.GetPins())
		{
			if (Pin.Type.IsWildcard())
			{
				bHasWildcardPin = true;
			}
		}

		if (bHasWildcardPin)
		{
			// Can't replace template with a wildcard
			// If this node is unused, it'll be removed by RemoveUnusedNodes down the pipeline
			// If it is used, AddWildcardErrors will catch it
			continue;
		}

		FVoxelTemplateNodeContext Context(Node.NodeRef);

		ensure(!GVoxelTemplateNodeContext);
		GVoxelTemplateNodeContext = &Context;
		ON_SCOPE_EXIT
		{
			ensure(GVoxelTemplateNodeContext == &Context);
			GVoxelTemplateNodeContext = nullptr;
		};

		InitializeTemplatesPassthroughNodes(Node);

		Node.GetVoxelNode<FVoxelTemplateNode>().ExpandNode(*this, Node);
		RemoveNode(Node);

		return true;
	}

	return false;
}

void FVoxelGraphCompiler::InitializeTemplatesPassthroughNodes(FNode& Node)
{
	VOXEL_FUNCTION_COUNTER();

	for (FPin& InputPin : Node.GetInputPins())
	{
		FNode& Passthrough = NewNode(ENodeType::Passthrough, FVoxelTemplateNodeUtilities::GetNodeRef());
		FPin& PassthroughInputPin = Passthrough.NewInputPin("Input" + InputPin.Name, InputPin.Type);
		FPin& PassthroughOutputPin = Passthrough.NewOutputPin(InputPin.Name, InputPin.Type);

		InputPin.CopyInputPinTo(PassthroughInputPin);

		InputPin.BreakAllLinks();
		InputPin.MakeLinkTo(PassthroughOutputPin);
	}

	for (FPin& OutputPin : Node.GetOutputPins())
	{
		FNode& Passthrough = NewNode(ENodeType::Passthrough, FVoxelTemplateNodeUtilities::GetNodeRef());
		FPin& PassthroughInputPin = Passthrough.NewInputPin(OutputPin.Name, OutputPin.Type);
		if (OutputPin.Type.HasPinDefaultValue())
		{
			PassthroughInputPin.SetDefaultValue(FVoxelPinValue(OutputPin.Type.GetPinDefaultValueType()));
		}
		FPin& PassthroughOutputPin = Passthrough.NewOutputPin("Output" + OutputPin.Name, OutputPin.Type);

		OutputPin.CopyOutputPinTo(PassthroughOutputPin);

		OutputPin.BreakAllLinks();
		OutputPin.MakeLinkTo(PassthroughInputPin);
	}
}

void FVoxelGraphCompiler::RemoveNodesImpl(const TVoxelArray<const FNode*>& RootNodes)
{
	VOXEL_FUNCTION_COUNTER();

	TSet<const FNode*> ValidNodes;

	TVoxelArray<const FNode*> NodesToVisit = RootNodes;
	while (NodesToVisit.Num() > 0)
	{
		const FNode* Node = NodesToVisit.Pop();
		if (ValidNodes.Contains(Node))
		{
			continue;
		}
		ValidNodes.Add(Node);

		for (const FPin& Pin : Node->GetInputPins())
		{
			for (const FPin& LinkedTo : Pin.GetLinkedTo())
			{
				NodesToVisit.Add(&LinkedTo.Node);
			}
		}
	}

	RemoveNodes([&](const FNode& Node)
	{
		return !ValidNodes.Contains(&Node);
	});
}