// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelParameterContainer_DEPRECATED.h"

void UVoxelParameterContainer_DEPRECATED::PostInitProperties()
{
	if (GVoxelDoNotCreateSubobjects)
	{
		UObject::PostInitProperties();
	}
	else
	{
		Super::PostInitProperties();
	}
}

void UVoxelParameterContainer_DEPRECATED::MigrateTo(FVoxelParameterOverrides& ParameterOverrides)
{
	ParameterOverrides.PathToValueOverride.Append(ValueOverrides);
	ValueOverrides.Empty();
}