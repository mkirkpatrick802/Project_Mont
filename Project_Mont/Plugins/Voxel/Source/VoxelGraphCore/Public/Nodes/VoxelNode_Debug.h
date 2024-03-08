// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelNode_Debug.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelDebugQueryParameter : public FVoxelQueryParameter
{
	GENERATED_BODY()
	GENERATED_VOXEL_QUERY_PARAMETER_BODY()

	struct FEntry
	{
		FVoxelGraphNodeRef NodeRef;
		FVoxelPinType Type;
		TSharedPtr<const FVoxelComputeValue> Compute;
	};

	mutable FVoxelCriticalSection CriticalSection;
	mutable TVoxelMap<FVoxelGraphNodeRef, FEntry> Entries_RequiresLock;
};

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_Debug : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelWildcard, In, nullptr, VirtualPin);
	VOXEL_OUTPUT_PIN(FVoxelWildcard, Out);
};