// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_GetPreviousChannelValue.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_GetPreviousChannelValue, Value)
{
	FindVoxelQueryParameter(FVoxelPreviousChannelValueQueryParameter, QueryParameter);

	if (QueryParameter->Value.GetType() != ReturnPinType)
	{
		VOXEL_MESSAGE(Error, "{0}: Channel has type {1}, but output pin has type {2}",
			this,
			QueryParameter->Value.GetType().ToString(),
			ReturnPinType.ToString());
		return {};
	}

	return QueryParameter->Value;
}

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_GetPreviousChannelValue, IsDefault)
{
	FindVoxelQueryParameter(FVoxelPreviousChannelValueQueryParameter, QueryParameter);
	return QueryParameter->bIsDefaultValue;
}