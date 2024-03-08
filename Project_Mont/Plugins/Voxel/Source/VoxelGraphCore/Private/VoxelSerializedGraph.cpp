// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelSerializedGraph.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphCompileScope.h"
#include "Nodes/VoxelNode_Output.h"
#include "Nodes/VoxelNode_UFunction.h"
#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#endif

FName FVoxelSerializedNode::GetNodeId() const
{
	if (!ensure(VoxelNode.IsValid()))
	{
		return {};
	}

	// NodeId needs to be unique
	// NodeId needs to avoid collision of nodes of different types: typically, function nodes with the same node name but different functions

	if (const FVoxelNode_Output* OutputNode = VoxelNode->As<FVoxelNode_Output>())
	{
		// Use Output.Name
		return FVoxelGraphConstants::GetOutputNodeId(OutputNode->Guid);
	}

	if (const FVoxelNode_UFunction* FunctionNode = VoxelNode->As<FVoxelNode_UFunction>())
	{
		if (const UFunction* Function = FunctionNode->GetFunction())
		{
			// Use Class.Function.NodeName
			return FName(Function->GetOuterUClass()->GetName() + "." + Function->GetName() + "." + EdGraphNodeName);
		}
	}

	// Use Struct.NodeName
	return FName(VoxelNode->GetStruct()->GetName() + "." + EdGraphNodeName);
}

FVoxelSerializedPin* FVoxelSerializedNode::FindPin(const FName PinName, const bool bIsInput)
{
	TMap<FName, FVoxelSerializedPin>& Pins = bIsInput ? InputPins : OutputPins;
	return Pins.Find(PinName);
}

TSharedRef<FVoxelMessageToken> FVoxelSerializedNode::CreateMessageToken() const
{
	VOXEL_FUNCTION_COUNTER();

#if !WITH_EDITOR
	return FVoxelMessageTokenFactory::CreateToken(EdGraphNodeTitle);
#else
	if (!ensure(IsInGameThread()) ||
		!ensure(GVoxelGraphCompileScope))
	{
		return FVoxelMessageTokenFactory::CreateToken(EdGraphNodeTitle);
	}

	const UEdGraph& EdGraph = GVoxelGraphCompileScope->TerminalGraph.GetEdGraph();
	for (const UEdGraphNode* EdGraphNode : EdGraph.Nodes)
	{
		if (EdGraphNode->GetFName() == EdGraphNodeName)
		{
			return FVoxelMessageTokenFactory::CreateToken(EdGraphNode);
		}
	}

	ensure(false);
	return FVoxelMessageTokenFactory::CreateToken(EdGraphNodeTitle);
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSerializedPin* FVoxelSerializedGraph::FindPin(const FVoxelSerializedPinRef& Ref)
{
	FVoxelSerializedNode* Node = NodeNameToNode.Find(Ref.NodeName);
	if (!Node)
	{
		return nullptr;
	}

	TMap<FName, FVoxelSerializedPin>& Pins = Ref.bIsInput ? Node->InputPins : Node->OutputPins;
	return Pins.Find(Ref.PinName);
}

const FVoxelNode* FVoxelSerializedGraph::FindMakeNode(const FVoxelPinType& Type) const
{
	const FVoxelInstancedStruct* Node = TypeToMakeNode.Find(Type);
	if (!ensure(Node) ||
		!ensure(Node->IsA<FVoxelNode>()))
	{
		return nullptr;
	}
	return &Node->Get<FVoxelNode>();
}

const FVoxelNode* FVoxelSerializedGraph::FindBreakNode(const FVoxelPinType& Type) const
{
	const FVoxelInstancedStruct* Node = TypeToBreakNode.Find(Type);
	if (!ensure(Node) ||
		!ensure(Node->IsA<FVoxelNode>()))
	{
		return nullptr;
	}
	return &Node->Get<FVoxelNode>();
}