// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntimePinValue.h"

struct VOXELGRAPHCORE_API FVoxelChannelDefinition
{
	FName Name;
	FVoxelPinType Type;
	FVoxelRuntimePinValue DefaultValue;

	FString ToString() const;
	FVoxelPinValue GetExposedDefaultValue() const;

	bool operator==(const FVoxelChannelDefinition& Other) const
	{
		return
			Name == Other.Name &&
			Type == Other.Type &&
			GetExposedDefaultValue() == Other.GetExposedDefaultValue();
	}
};