// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphEditorInterface.h"
#include "VoxelGraph.h"
#include "VoxelEdGraph.h"
#include "VoxelGraphSchema.h"
#include "VoxelNodeLibrary.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "Nodes/VoxelGraphNode_Knot.h"
#include "Nodes/VoxelGraphNode_Input.h"
#include "Nodes/VoxelGraphNode_Output.h"
#include "Nodes/VoxelGraphNode_Struct.h"
#include "Nodes/VoxelGraphNode_Parameter.h"
#include "Nodes/VoxelGraphNode_CallGraph.h"
#include "Nodes/VoxelGraphNode_CallFunction.h"
#include "Nodes/VoxelGraphNode_LocalVariable.h"
#include "Nodes/VoxelInputNodes.h"
#include "Nodes/VoxelNode_Output.h"
#include "Nodes/VoxelNode_Parameter.h"
#include "Nodes/VoxelCallGraphNodes.h"
#include "Nodes/VoxelCallFunctionNodes.h"
#include "Nodes/VoxelLocalVariableNodes.h"
#include "Subsystems/AssetEditorSubsystem.h"

VOXEL_RUN_ON_STARTUP_EDITOR_COMMANDLET(RegisterGraphEditorInterface)
{
	check(!GVoxelGraphEditorInterface);
	GVoxelGraphEditorInterface = new FVoxelGraphEditorInterface();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphEditorInterface::CompileAll()
{
	VOXEL_FUNCTION_COUNTER();

	UVoxelGraph::LoadAllGraphs();

	TArray<FSoftObjectPath> AssetsToOpen;
	ForEachObjectOfClass_Copy<UVoxelGraph>([&](UVoxelGraph& Graph)
	{
		bool bShouldOpen = false;

		Graph.ForeachTerminalGraph_NoInheritance([&](const UVoxelTerminalGraph& TerminalGraph)
		{
			UVoxelTerminalGraphRuntime& Runtime = TerminalGraph.GetRuntime();
			Runtime.EnsureIsCompiled();

			if (Runtime.HasCompileMessages())
			{
				bShouldOpen = true;
			}
		});

		if (bShouldOpen)
		{
			AssetsToOpen.Add(&Graph);
		}
	});

	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorsForAssets(AssetsToOpen);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphEditorInterface::ReconstructAllNodes(UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();

	TerminalGraph.GetTypedEdGraph().MigrateAndReconstructAll();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelGraphEditorInterface::HasNode(const UVoxelTerminalGraph& TerminalGraph, const UScriptStruct* Struct)
{
	VOXEL_FUNCTION_COUNTER();

	// Important: don't migrate, we can end up calling HasExecNodes recursively otherwise

	for (const UEdGraphNode* EdGraphNode : TerminalGraph.GetEdGraph().Nodes)
	{
		const UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(EdGraphNode);
		if (!Node ||
			Node->IsA<UVoxelGraphNode_Knot>())
		{
			continue;
		}

		if (const UVoxelGraphNode_Struct* GraphNode = Cast<UVoxelGraphNode_Struct>(EdGraphNode))
		{
			if (GraphNode->Struct.IsA(Struct))
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_CallFunction>())
		{
			if (Struct == StaticStructFast<FVoxelExecNode_CallFunction>())
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_CallExternalFunction>())
		{
			if (Struct == StaticStructFast<FVoxelExecNode_CallExternalFunction>())
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_CallGraph>())
		{
			if (Struct == StaticStructFast<FVoxelExecNode_CallGraph>())
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_CallGraphParameter>())
		{
			if (Struct == StaticStructFast<FVoxelExecNode_CallGraphParameter>())
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_Parameter>())
		{
			if (Struct == StaticStructFast<FVoxelNode_Parameter>())
			{
				return true;
			}
			continue;
		}

		if (const UVoxelGraphNode_Input* GraphNode = Cast<UVoxelGraphNode_Input>(EdGraphNode))
		{
			if (GraphNode->bExposeDefaultPin)
			{
				if (Struct == StaticStructFast<FVoxelNode_Input_WithDefaultPin>())
				{
					return true;
				}
			}
			else
			{
				if (Struct == StaticStructFast<FVoxelNode_Input_WithoutDefaultPin>())
				{
					return true;
				}
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_Output>())
		{
			if (Struct == StaticStructFast<FVoxelNode_Output>())
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_LocalVariableDeclaration>())
		{
			if (Struct == StaticStructFast<FVoxelNode_LocalVariableDeclaration>())
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UVoxelGraphNode_LocalVariableUsage>())
		{
			if (Struct == StaticStructFast<FVoxelNode_LocalVariableUsage>())
			{
				return true;
			}
			continue;
		}

		if (EdGraphNode->IsA<UDEPRECATED_VoxelGraphNode_Macro>() ||
			EdGraphNode->IsA<UDEPRECATED_VoxelGraphMacroParameterNode>())
		{
			continue;
		}

		ensure(false);
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UEdGraph* FVoxelGraphEditorInterface::CreateEdGraph(UVoxelTerminalGraph& TerminalGraph)
{
	UVoxelEdGraph* EdGraph = NewObject<UVoxelEdGraph>(&TerminalGraph, NAME_None, RF_Transactional);
	EdGraph->Schema = UVoxelGraphSchema::StaticClass();
	return EdGraph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSerializedGraph FVoxelGraphEditorInterface::Translate(UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();

	UVoxelEdGraph& EdGraph = TerminalGraph.GetTypedEdGraph();
	EdGraph.MigrateIfNeeded();

	// Fixup reroute nodes
	for (UEdGraphNode* GraphNode : EdGraph.Nodes)
	{
		if (UVoxelGraphNode_Knot* Node = Cast<UVoxelGraphNode_Knot>(GraphNode))
		{
			Node->PropagatePinType();
		}
	}

	// Sanitize nodes, we've had cases where some nodes are null
	EdGraph.Nodes.RemoveAllSwap([](const UEdGraphNode* Node)
	{
		return !IsValid(Node);
	});

	TVoxelArray<const UVoxelGraphNode*> Nodes;
	for (const UEdGraphNode* EdGraphNode : EdGraph.Nodes)
	{
		const UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(EdGraphNode);
		if (!Node ||
			Node->IsA<UVoxelGraphNode_Knot>())
		{
			continue;
		}
		Nodes.Add(Node);

		for (const UEdGraphPin* Pin : Node->Pins)
		{
			const auto GetError = [&]
			{
				return
					Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString() +
					" " +
					Node->GetPathName();
			};

			if (!ensureAlwaysMsgf(Pin, TEXT("Invalid pin on %s"), *GetError()))
			{
				return {};
			}

			if (Pin->ParentPin)
			{
				if (!ensureAlwaysMsgf(Pin->ParentPin->SubPins.Contains(Pin), TEXT("Invalid sub-pin on %s"), *GetError()))
				{
					return {};
				}
			}

			for (const UEdGraphPin* SubPin : Pin->SubPins)
			{
				if (!ensureAlwaysMsgf(SubPin->ParentPin == Pin, TEXT("Invalid parent pin on %s"), *GetError()))
				{
					return {};
				}
			}

			for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!ensureAlwaysMsgf(LinkedPin, TEXT("Invalid pin linked to %s"), *GetError()))
				{
					return {};
				}

				if (!ensureAlwaysMsgf(LinkedPin->LinkedTo.Contains(Pin), TEXT("Invalid link on %s"), *GetError()))
				{
					return {};
				}
			}
		}
	}

	FVoxelSerializedGraph SerializedGraph;
	SerializedGraph.Version = FVoxelSerializedGraph::FVersion::LatestVersion;

	for (const UEdGraphNode* EdGraphNode : Nodes)
	{
		FVoxelSerializedNode SerializedNode;
		SerializedNode.VoxelNode = INLINE_LAMBDA -> TVoxelInstancedStruct<FVoxelNode>
		{
			if (const UVoxelGraphNode_Struct* GraphNode = Cast<UVoxelGraphNode_Struct>(EdGraphNode))
			{
				return GraphNode->Struct;
			}

			if (const UVoxelGraphNode_CallFunction* GraphNode = Cast<UVoxelGraphNode_CallFunction>(EdGraphNode))
			{
				FVoxelExecNode_CallFunction Node;
				Node.Guid = GraphNode->Guid;

				if (GraphNode->bCallParent)
				{
					UVoxelGraph* BaseGraph = TerminalGraph.GetGraph().GetBaseGraph_Unsafe();
					if (!BaseGraph)
					{
						SerializedNode.Errors.Add("Invalid function call: Base Graph is null");
					}
					Node.ContextOverride = BaseGraph;
				}

				Node.FixupPins(TerminalGraph.GetGraph(), FOnVoxelGraphChanged::Null());
				return Node;
			}

			if (const UVoxelGraphNode_CallExternalFunction* GraphNode = Cast<UVoxelGraphNode_CallExternalFunction>(EdGraphNode))
			{
				FVoxelExecNode_CallExternalFunction Node;
				Node.FunctionLibrary = GraphNode->FunctionLibrary;
				Node.Guid = GraphNode->Guid;
				Node.FixupPins(TerminalGraph.GetGraph(), FOnVoxelGraphChanged::Null());
				return Node;
			}

			if (const UVoxelGraphNode_CallGraph* GraphNode = Cast<UVoxelGraphNode_CallGraph>(EdGraphNode))
			{
				FVoxelExecNode_CallGraph Node;
				Node.Graph = GraphNode->Graph;
				Node.FixupPins(TerminalGraph.GetGraph(), FOnVoxelGraphChanged::Null());
				return Node;
			}

			if (const UVoxelGraphNode_CallGraphParameter* GraphNode = Cast<UVoxelGraphNode_CallGraphParameter>(EdGraphNode))
			{
				FVoxelExecNode_CallGraphParameter Node;
				Node.BaseGraph = GraphNode->BaseGraph;
				Node.FixupPins(TerminalGraph.GetGraph(), FOnVoxelGraphChanged::Null());
				return Node;
			}

			if (const UVoxelGraphNode_Parameter* GraphNode = Cast<UVoxelGraphNode_Parameter>(EdGraphNode))
			{
				const FVoxelParameter Parameter = GraphNode->GetParameterSafe();

				FVoxelNode_Parameter Node;
				Node.ParameterGuid = GraphNode->Guid;
				Node.ParameterName = Parameter.Name;
				Node.GetPin(Node.ValuePin).SetType(Parameter.Type);
				return Node;
			}

			if (const UVoxelGraphNode_Input* GraphNode = Cast<UVoxelGraphNode_Input>(EdGraphNode))
			{
				const FVoxelGraphInput Input = GraphNode->GetInputSafe();

				if (GraphNode->bExposeDefaultPin)
				{
					FVoxelNode_Input_WithDefaultPin Node;
					Node.Guid = GraphNode->Guid;
					Node.bIsGraphInput = GraphNode->bIsGraphInput;
					Node.GetPin(Node.DefaultPin).SetType(Input.Type);
					Node.GetPin(Node.ValuePin).SetType(Input.Type);
					return Node;
				}
				else
				{
					FVoxelNode_Input_WithoutDefaultPin Node;
					Node.Guid = GraphNode->Guid;
					Node.bIsGraphInput = GraphNode->bIsGraphInput;
					Node.DefaultValue = Input.DefaultValue;
					Node.GetPin(Node.ValuePin).SetType(Input.Type);
					return Node;
				}
			}

			if (const UVoxelGraphNode_Output* GraphNode = Cast<UVoxelGraphNode_Output>(EdGraphNode))
			{
				// Add that output to the graph so we know to query it
				SerializedGraph.OutputGuids.Add(GraphNode->Guid);

				const FVoxelGraphOutput Output = GraphNode->GetOutputSafe();

				FVoxelNode_Output Node;
				Node.Guid = GraphNode->Guid;
				Node.GetPin(Node.ValuePin).SetType(Output.Type);
				return Node;
			}

			if (const UVoxelGraphNode_LocalVariableDeclaration* GraphNode = Cast<UVoxelGraphNode_LocalVariableDeclaration>(EdGraphNode))
			{
				const FVoxelGraphLocalVariable LocalVariable = GraphNode->GetLocalVariableSafe();

				FVoxelNode_LocalVariableDeclaration Node;
				Node.Guid = GraphNode->Guid;
				Node.GetPin(Node.InputPinPin).SetType(LocalVariable.Type);
				Node.GetPin(Node.OutputPinPin).SetType(LocalVariable.Type);
				return Node;
			}

			if (const UVoxelGraphNode_LocalVariableUsage* GraphNode = Cast<UVoxelGraphNode_LocalVariableUsage>(EdGraphNode))
			{
				const FVoxelGraphLocalVariable LocalVariable = GraphNode->GetLocalVariableSafe();

				FVoxelNode_LocalVariableUsage Node;
				Node.Guid = GraphNode->Guid;
				Node.GetPin(Node.OutputPinPin).SetType(LocalVariable.Type);
				return Node;
			}

			if (EdGraphNode->IsA<UDEPRECATED_VoxelGraphNode_Macro>() ||
				EdGraphNode->IsA<UDEPRECATED_VoxelGraphMacroParameterNode>())
			{
				// SerializedGraph will automatically migrate on failure
				return {};
			}

			ensure(false);
			return {};
		};

		if (SerializedNode.VoxelNode)
		{
			SerializedNode.StructName = SerializedNode.VoxelNode.GetScriptStruct()->GetFName();
		}

		SerializedNode.EdGraphNodeName = EdGraphNode->GetFName();
		SerializedNode.EdGraphNodeTitle = *EdGraphNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

		ensure(!SerializedGraph.NodeNameToNode.Contains(SerializedNode.EdGraphNodeName));
		SerializedGraph.NodeNameToNode.Add(SerializedNode.EdGraphNodeName, SerializedNode);
	}

	TMap<const UEdGraphPin*, FVoxelSerializedPinRef> EdGraphPinToPinRef;
	for (const UVoxelGraphNode* EdGraphNode : Nodes)
	{
		FVoxelSerializedNode& SerializedNode = SerializedGraph.NodeNameToNode[EdGraphNode->GetFName()];

		const auto MakePin = [&](const UEdGraphPin* EdGraphPin)
		{
			ensure(!EdGraphPin->bOrphanedPin);

			FVoxelSerializedPin SerializedPin;
			SerializedPin.Type = EdGraphPin->PinType;
			SerializedPin.PinName = EdGraphPin->PinName;

			for (const UEdGraphPin* Pin = EdGraphPin->ParentPin; Pin; Pin = Pin->ParentPin)
			{
				SerializedPin.PinName = Pin->PinName.ToString() + "|" + SerializedPin.PinName;
			}

			if (const UEdGraphPin* ParentPin = EdGraphPin->ParentPin)
			{
				const FVoxelPinType Type(ParentPin->PinType);
				if (ParentPin->Direction == EGPD_Input)
				{
					SerializedGraph.TypeToMakeNode.Add(Type);
				}
				else
				{
					check(ParentPin->Direction == EGPD_Output);
					SerializedGraph.TypeToBreakNode.Add(Type);
				}

				SerializedPin.ParentPinName = ParentPin->PinName;
			}

			if (SerializedPin.Type.HasPinDefaultValue())
			{
				SerializedPin.DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*EdGraphPin);
			}
			else
			{
				ensureVoxelSlow(!EdGraphPin->DefaultObject);
				ensureVoxelSlow(EdGraphPin->DefaultValue.IsEmpty());
				ensureVoxelSlow(EdGraphPin->AutogeneratedDefaultValue.IsEmpty());
			}

			if (EdGraphPin->PinName == FVoxelGraphConstants::PinName_EnableNode)
			{
				if (const UVoxelGraphNode_CallTerminalGraph* CallTerminalGraphNode = Cast<UVoxelGraphNode_CallTerminalGraph>(EdGraphPin->GetOwningNode()))
				{
					if (const UVoxelTerminalGraph* BaseTerminalGraph = CallTerminalGraphNode->GetBaseTerminalGraph())
					{
						if (!BaseTerminalGraph->HasExecNodes())
						{
							// Make sure to never execute any graph parameter we might end up being assigned with
							ensure(EdGraphPin->LinkedTo.Num() == 0);
							SerializedPin.DefaultValue.Get<bool>() = false;
						}
					}
				}
			}

			return SerializedPin;
		};

		for (const UEdGraphPin* Pin : EdGraphNode->GetInputPins())
		{
			if (Pin->bOrphanedPin)
			{
				SerializedNode.Errors.Add("Orphaned pin " + Pin->GetDisplayName().ToString());
				continue;
			}
			if (!ensureVoxelSlow(FVoxelPinType(Pin->PinType).IsValid()))
			{
				SerializedNode.Errors.Add("Pin with invalid type " + Pin->GetDisplayName().ToString());
				continue;
			}

			if (!ensure(!SerializedNode.InputPins.Contains(Pin->PinName)))
			{
				continue;
			}

			FVoxelSerializedPin NewPin = MakePin(Pin);
			SerializedNode.InputPins.Add(NewPin.PinName, NewPin);
			EdGraphPinToPinRef.Add(Pin, FVoxelSerializedPinRef{ EdGraphNode->GetFName(), NewPin.PinName, true });
		}
		for (const UEdGraphPin* Pin : EdGraphNode->GetOutputPins())
		{
			if (Pin->bOrphanedPin)
			{
				// Don't add compile error for output pins
				SerializedNode.Warnings.Add("Orphaned pin " + Pin->GetDisplayName().ToString());
				continue;
			}
			if (!ensureVoxelSlow(FVoxelPinType(Pin->PinType).IsValid()))
			{
				// Don't add compile error for output pins
				SerializedNode.Warnings.Add("Pin with invalid type " + Pin->GetDisplayName().ToString());
				continue;
			}

			if (!ensure(!SerializedNode.OutputPins.Contains(Pin->PinName)))
			{
				continue;
			}

			FVoxelSerializedPin NewPin = MakePin(Pin);
			SerializedNode.OutputPins.Add(NewPin.PinName, NewPin);
			EdGraphPinToPinRef.Add(Pin, FVoxelSerializedPinRef{ EdGraphNode->GetFName(), NewPin.PinName, false });
		}
	}

	for (auto& It : SerializedGraph.TypeToMakeNode)
	{
		const TSharedPtr<const FVoxelNode> Node = FVoxelNodeLibrary::FindMakeNode(It.Key);
		if (!ensure(Node))
		{
			continue;
		}

		It.Value = FVoxelInstancedStruct::Make(*Node);
	}
	for (auto& It : SerializedGraph.TypeToBreakNode)
	{
		const TSharedPtr<const FVoxelNode> Node = FVoxelNodeLibrary::FindBreakNode(It.Key);
		if (!ensure(Node))
		{
			continue;
		}

		It.Value = FVoxelInstancedStruct::Make(*Node);
	}

	// Link the pins once they're all allocated
	for (const UVoxelGraphNode* EdGraphNode : Nodes)
	{
		FVoxelSerializedNode* SerializedNode = SerializedGraph.NodeNameToNode.Find(EdGraphNode->GetFName());
		if (!ensure(SerializedNode))
		{
			continue;
		}

		const auto AddPin = [&](const UEdGraphPin* Pin)
		{
			if (!ensure(EdGraphPinToPinRef.Contains(Pin)))
			{
				return false;
			}

			FVoxelSerializedPin* SerializedPin = SerializedGraph.FindPin(EdGraphPinToPinRef[Pin]);
			if (!ensure(SerializedPin))
			{
				return false;
			}

			for (const UEdGraphPin* LinkedTo : Pin->LinkedTo)
			{
				if (LinkedTo->bOrphanedPin)
				{
					if (Pin->Direction == EGPD_Input)
					{
						SerializedNode->Errors.Add(Pin->GetDisplayName().ToString() + " is connected to an orphaned pin");
					}
					continue;
				}

				if (UVoxelGraphNode_Knot* Knot = Cast<UVoxelGraphNode_Knot>(LinkedTo->GetOwningNode()))
				{
					for (const UEdGraphPin* LinkedToKnot : Knot->GetLinkedPins(Pin->Direction))
					{
						if (LinkedToKnot->bOrphanedPin)
						{
							continue;
						}

						if (ensure(EdGraphPinToPinRef.Contains(LinkedToKnot)))
						{
							SerializedPin->LinkedTo.Add(EdGraphPinToPinRef[LinkedToKnot]);
						}
					}
					continue;
				}

				if (ensure(EdGraphPinToPinRef.Contains(LinkedTo)))
				{
					SerializedPin->LinkedTo.Add(EdGraphPinToPinRef[LinkedTo]);
				}
			}

			return true;
		};

		for (const UEdGraphPin* Pin : EdGraphNode->GetInputPins())
		{
			if (Pin->bOrphanedPin)
			{
				continue;
			}

			if (!ensure(AddPin(Pin)))
			{
				return {};
			}
		}
		for (const UEdGraphPin* Pin : EdGraphNode->GetOutputPins())
		{
			if (Pin->bOrphanedPin)
			{
				continue;
			}

			if (!ensure(AddPin(Pin)))
			{
				return {};
			}
		}
	}

	for (const UVoxelGraphNode* Node : Nodes)
	{
		if (!Node->bEnableDebug)
		{
			continue;
		}

		SerializedGraph.DebuggedNodes.Add(Node->GetFName());
	}

	INLINE_LAMBDA
	{
		VOXEL_SCOPE_COUNTER("Setup preview");

		TVoxelArray<UEdGraphPin*> PreviewedPins;
		for (const UVoxelGraphNode* Node : Nodes)
		{
			if (!Node->bEnablePreview)
			{
				continue;
			}

			UEdGraphPin* PreviewedPin = Node->FindPin(Node->PreviewedPin);
			if (!PreviewedPin)
			{
				continue;
			}

			PreviewedPins.Add(PreviewedPin);
		}

		if (PreviewedPins.Num() > 1)
		{
			for (const UEdGraphPin* PreviewedPin : PreviewedPins)
			{
				CastChecked<UVoxelGraphNode>(PreviewedPin->GetOwningNode())->bEnablePreview = false;
			}
			return;
		}

		if (PreviewedPins.Num() == 0)
		{
			return;
		}

		const UEdGraphPin& PreviewedPin = *PreviewedPins[0];
		UVoxelGraphNode* PreviewedNode = CastChecked<UVoxelGraphNode>(PreviewedPin.GetOwningNode());

		FVoxelInstancedStruct PreviewHandler;
		for (const FVoxelPinPreviewSettings& PreviewSettings : PreviewedNode->PreviewSettings)
		{
			if (PreviewSettings.PinName == PreviewedPin.PinName)
			{
				ensure(!PreviewHandler.IsValid());
				PreviewHandler = PreviewSettings.PreviewHandler;
			}
		}
		if (!PreviewHandler.IsValid())
		{
			return;
		}

		SerializedGraph.PreviewedPin = FVoxelSerializedPinRef
		{
			PreviewedNode->GetFName(),
			PreviewedPin.PinName,
			PreviewedPin.Direction == EGPD_Input
		};

		SerializedGraph.PreviewedPinType = PreviewedPin.PinType;
		SerializedGraph.PreviewHandler = PreviewHandler;
	};

	SerializedGraph.bIsValid = true;
	return SerializedGraph;
}