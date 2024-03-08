// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class FVoxelRuntimeChannel;

class VOXELGRAPHCORE_API FVoxelRuntimeChannelCache : public TSharedFromThis<FVoxelRuntimeChannelCache>
{
public:
	static TSharedRef<FVoxelRuntimeChannelCache> Create();

private:
	FVoxelCriticalSection CriticalSection;
	TVoxelMap<FName, TSharedPtr<FVoxelRuntimeChannel>> Channels_RequiresLock;

	FVoxelRuntimeChannelCache() = default;

	friend class FVoxelWorldChannel;
};