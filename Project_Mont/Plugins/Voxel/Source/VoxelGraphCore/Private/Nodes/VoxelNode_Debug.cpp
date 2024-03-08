// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_Debug.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_Debug, Out)
{
	if (const FVoxelDebugQueryParameter* DebugQueryParameter = Query.GetParameters().Find<FVoxelDebugQueryParameter>())
	{
		VOXEL_SCOPE_LOCK(DebugQueryParameter->CriticalSection);

		DebugQueryParameter->Entries_RequiresLock.FindOrAdd(GetNodeRef()) = FVoxelDebugQueryParameter::FEntry
		{
			GetNodeRef(),
				ReturnPinType,
				GetCompute(InPin, Query.GetSharedTerminalGraphInstance())
		};
	}

	return Get(InPin, Query);
}