// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSubsystem.h"
#include "Channel/VoxelRuntimeChannelCache.h"
#include "VoxelChannelSubsystem.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelChannelSubsystem : public FVoxelSubsystem
{
	GENERATED_BODY()
	GENERATED_VOXEL_SUBSYSTEM_BODY(FVoxelChannelSubsystem)

	TSharedPtr<FVoxelRuntimeChannelCache> Cache;

	virtual void Create() override
	{
		Cache = FVoxelRuntimeChannelCache::Create();
	}
};