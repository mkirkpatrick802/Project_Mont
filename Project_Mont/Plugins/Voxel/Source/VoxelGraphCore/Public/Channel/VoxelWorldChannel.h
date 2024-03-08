// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTransformRef.h"
#include "Channel/VoxelBrush.h"
#include "Channel/VoxelChannelDefinition.h"
#include "Channel/VoxelRuntimeChannelCache.h"

class FVoxelWorldChannel;

class VOXELGRAPHCORE_API FVoxelBrushRef
{
public:
	~FVoxelBrushRef();

private:
	TWeakPtr<FVoxelWorldChannel> WeakChannel;
	FVoxelBrushId BrushId;

	FVoxelBrushRef(const TSharedRef<FVoxelWorldChannel>& Channel, const FVoxelBrushId BrushId)
		: WeakChannel(Channel)
		, BrushId(BrushId)
	{
	}

	friend FVoxelWorldChannel;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelWorldChannel : public TSharedFromThis<FVoxelWorldChannel>
{
public:
	const FVoxelChannelDefinition Definition;

	void AddBrush(
		const TSharedRef<const FVoxelBrush>& Brush,
		TSharedPtr<FVoxelBrushRef>& OutBrushRef);

	TSharedRef<FVoxelRuntimeChannel> GetRuntimeChannel(
		const FVoxelTransformRef& RuntimeLocalToWorld,
		FVoxelRuntimeChannelCache& Cache);

	void DrawBrushBounds(
		FObjectKey World,
		const FLinearColor& Color) const;

private:
	mutable FVoxelCriticalSection CriticalSection;
	TVoxelArray<TWeakPtr<FVoxelRuntimeChannel>> WeakRuntimeChannels_RequiresLock;
	TVoxelSparseArray<TSharedPtr<const FVoxelBrush>, FVoxelBrushId> Brushes_RequiresLock;

	explicit FVoxelWorldChannel(const FVoxelChannelDefinition& Definition)
		: Definition(Definition)
	{
	}

	void RemoveBrush_RequiresLock(FVoxelBrushId BrushId);

	friend class FVoxelBrushRef;
	friend class FVoxelChannelManager;
	friend class FVoxelWorldChannelManager;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelWorldChannelManager : public IVoxelWorldSubsystem
{
public:
	GENERATED_VOXEL_WORLD_SUBSYSTEM_BODY(FVoxelWorldChannelManager);

	bool RegisterChannel(const FVoxelChannelDefinition& ChannelDefinition);
	TSharedPtr<FVoxelWorldChannel> FindChannel(FName Name);
	TVoxelArray<FName> GetValidChannelNames() const;

	//~ Begin IVoxelWorldSubsystem Interface
	virtual void Tick() override;
	//~ End IVoxelWorldSubsystem Interface

public:
	static TSharedPtr<FVoxelRuntimeChannel> FindRuntimeChannel(
		const UWorld* World,
		FName ChannelName,
		const FVoxelTransformRef& Transform = FVoxelTransformRef::Identity(),
		const TSharedRef<FVoxelRuntimeChannelCache>& Cache = FVoxelRuntimeChannelCache::Create());

private:
	mutable FVoxelCriticalSection CriticalSection;
	TVoxelMap<FName, TSharedPtr<FVoxelWorldChannel>> Channels_RequiresLock;

	friend class FVoxelChannelManager;
};