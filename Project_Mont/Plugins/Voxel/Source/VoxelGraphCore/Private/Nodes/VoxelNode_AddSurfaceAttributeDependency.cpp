// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_AddSurfaceAttributeDependency.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_AddSurfaceAttributeDependency, Out)
{
	const TValue<FName> Attribute = Get(AttributePin, Query);
	const TValue<FName> Dependency = Get(DependencyPin, Query);

	return VOXEL_ON_COMPLETE(Attribute, Dependency)
	{
		FindOptionalVoxelQueryParameter(FVoxelSurfaceQueryParameter, SurfaceQueryParameter);

		if (!SurfaceQueryParameter ||
			!SurfaceQueryParameter->ShouldCompute(Attribute) ||
			SurfaceQueryParameter->ShouldCompute(Dependency))
		{
			return Get(InPin, Query);
		}

		const TSharedRef<FVoxelSurfaceQueryParameter> NewSurfaceQueryParameter = MakeVoxelShared<FVoxelSurfaceQueryParameter>();
		if (SurfaceQueryParameter)
		{
			NewSurfaceQueryParameter->AttributesToCompute = SurfaceQueryParameter->AttributesToCompute;
		}
		NewSurfaceQueryParameter->AttributesToCompute.Add_CheckNew(Dependency);

		return Get(InPin, Query.MakeNewQuery(NewSurfaceQueryParameter));
	};
}