// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_Parameter.h"
#include "VoxelTerminalGraphInstance.h"
#include "VoxelParameterOverridesRuntime.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_Parameter, Value)
{
	const TSharedPtr<const FVoxelParameterOverridesRuntime> ParameterOverridesRuntime = Query.GetTerminalGraphInstance().ParameterOverridesRuntime;
	if (!ensure(ParameterOverridesRuntime))
	{
		return {};
	}

	return ParameterOverridesRuntime->FindParameter(
		ReturnPinType,
		Query.GetTerminalGraphInstance().ParameterPath.MakeChild(ParameterGuid),
		Query);
}