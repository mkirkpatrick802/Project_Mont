// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_CallGraph.h"
#include "VoxelGraph.h"
#include "VoxelGraphNodeRef.h"
#include "VoxelInlineGraph.h"

const UVoxelTerminalGraph* UVoxelGraphNode_CallGraph::GetBaseTerminalGraph() const
{
	if (!Graph)
	{
		return nullptr;
	}
	return &Graph->GetMainTerminalGraph();
}

void UVoxelGraphNode_CallGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(Graph))
	{
		ReconstructNode();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_CallGraphParameter::AddPins()
{
	UEdGraphPin* Pin = CreatePin(
		EGPD_Input,
		FVoxelPinType::Make<FVoxelInlineGraph>().GetEdGraphPinType(),
		FVoxelGraphConstants::PinName_GraphParameter);

	Pin->PinFriendlyName = INVTEXT("Graph Parameter");
}

const UVoxelTerminalGraph* UVoxelGraphNode_CallGraphParameter::GetBaseTerminalGraph() const
{
	if (!BaseGraph)
	{
		return nullptr;
	}
	return &BaseGraph->GetMainTerminalGraph();
}

void UVoxelGraphNode_CallGraphParameter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(BaseGraph))
	{
		ReconstructNode();
	}
}