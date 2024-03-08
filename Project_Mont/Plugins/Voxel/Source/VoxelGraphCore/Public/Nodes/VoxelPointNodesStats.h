// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNodeInterface.h"

class VOXELGRAPHCORE_API FVoxelPointFilterStats
{
public:
	static void RecordNodeStats(const IVoxelNodeInterface& Node, int64 Count, int64 RemainingCount);
};