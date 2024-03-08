// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameterPath.h"
#include "VoxelInlineGraph.generated.h"

class UVoxelGraph;

USTRUCT(meta = (VoxelPinType))
struct VOXELGRAPHCORE_API FVoxelInlineGraph
{
	GENERATED_BODY()

	TWeakObjectPtr<UVoxelGraph> Graph;
	TOptional<FVoxelParameterPath> ParameterPath;

	bool operator==(const FVoxelInlineGraph& Other) const
	{
		return
			Graph == Other.Graph &&
			ParameterPath == Other.ParameterPath;
	}
};