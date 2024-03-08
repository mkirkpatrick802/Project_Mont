// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_RaiseError.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_RaiseError, Out)
{
	const TValue<FName> Error = Get(ErrorPin, Query);

	return VOXEL_ON_COMPLETE(Error)
	{
		VOXEL_MESSAGE(Error, "{0}: {1}", this, Error);
		return Get(InPin, Query);
	};
}

#if WITH_EDITOR
void FVoxelNode_RaiseError::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(InPin).SetType(NewType);
	GetPin(OutPin).SetType(NewType);
}
#endif