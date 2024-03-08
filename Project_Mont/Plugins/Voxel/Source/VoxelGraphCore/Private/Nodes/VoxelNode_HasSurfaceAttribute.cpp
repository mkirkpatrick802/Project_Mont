// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_HasSurfaceAttribute.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_HasSurfaceAttribute, ReturnValue)
{
	const TValue<FVoxelSurface> Surface = Get(SurfacePin, Query);
	const TValue<FName> Attribute = Get(AttributePin, Query);

	return VOXEL_ON_COMPLETE(Surface, Attribute)
	{
		return Surface->HasAttribute(Attribute);
	};
}