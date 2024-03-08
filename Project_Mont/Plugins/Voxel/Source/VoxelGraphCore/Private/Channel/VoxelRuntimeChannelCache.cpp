// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Channel/VoxelRuntimeChannelCache.h"
#include "Channel/VoxelChannelManager.h"

TSharedRef<FVoxelRuntimeChannelCache> FVoxelRuntimeChannelCache::Create()
{
	ensure(IsInGameThread());

	const TSharedRef<FVoxelRuntimeChannelCache> Cache = MakeVoxelShareable(new (GVoxelMemory) FVoxelRuntimeChannelCache());
	GVoxelChannelManager->OnChannelDefinitionsChanged_GameThread.Add(MakeWeakPtrDelegate(Cache, [&Cache = *Cache]
	{
		ensure(IsInGameThread());
		VOXEL_SCOPE_LOCK(Cache.CriticalSection);
		Cache.Channels_RequiresLock.Reset();
	}));
	return Cache;
}