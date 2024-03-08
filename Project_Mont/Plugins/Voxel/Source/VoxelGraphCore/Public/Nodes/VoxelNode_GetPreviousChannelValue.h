// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_GetPreviousChannelValue.generated.h"

USTRUCT()
struct FVoxelPreviousChannelValueQueryParameter : public FVoxelQueryParameter
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_PARAMETER_BODY()

	bool bIsDefaultValue = false;
	FVoxelRuntimePinValue Value;

	void Initialize(
		const bool bIsPreviousDefault,
		const FVoxelRuntimePinValue& PreviousValue)
	{
		bIsDefaultValue = bIsPreviousDefault;
		Value = PreviousValue;
	}
};

USTRUCT(Category = "Channel|Advanced")
struct VOXELGRAPHCORE_API FVoxelNode_GetPreviousChannelValue : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_OUTPUT_PIN(FVoxelWildcard, Value);
	// If true this isn't a value outputted by a brush but the default channel value
	// Will be false if only part of the values are default
	VOXEL_OUTPUT_PIN(bool, IsDefault);
};