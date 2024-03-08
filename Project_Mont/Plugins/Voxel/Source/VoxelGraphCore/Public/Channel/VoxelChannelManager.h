// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Channel/VoxelChannelDefinition.h"

struct FStreamableHandle;
struct FVoxelChannelExposedDefinition;

class VOXELGRAPHCORE_API FVoxelChannelManager : public FVoxelSingleton
{
public:
	FSimpleMulticastDelegate OnChannelDefinitionsChanged_GameThread;

	bool IsReady(bool bLog) const;

	TArray<FName> GetValidChannelNames() const;
	TArray<const UObject*> GetChannelAssets() const;
	TOptional<FVoxelChannelDefinition> FindChannelDefinition(FName Name) const;

	void LogAllBrushes_GameThread();
	void LogAllChannels_GameThread();

	bool RegisterChannel(const FVoxelChannelDefinition& ChannelDefinition);
	void UpdateChannelsFromAsset_GameThread(
		const UObject* Asset,
		const FString& Prefix,
		const TArray<FVoxelChannelExposedDefinition>& Channels);
	void RemoveChannelsFromAsset_GameThread(const UObject* Asset);

	void ClearQueuedRefresh()
	{
		bRefreshQueued = false;
	}

	//~ Begin FVoxelSingleton Interface
	virtual void Initialize() override;
	virtual void Tick() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End FVoxelSingleton Interface

private:
	bool bRefreshQueued = false;
	bool bChannelRegistriesLoaded = false;
	TVoxelArray<TSharedPtr<FStreamableHandle>> PendingHandles;

	mutable FVoxelCriticalSection CriticalSection;
	TVoxelMap<TObjectPtr<const UObject>, TVoxelMap<FName, FVoxelChannelDefinition>> AssetToChannelDefinitions_RequiresLock;
};
extern VOXELGRAPHCORE_API FVoxelChannelManager* GVoxelChannelManager;