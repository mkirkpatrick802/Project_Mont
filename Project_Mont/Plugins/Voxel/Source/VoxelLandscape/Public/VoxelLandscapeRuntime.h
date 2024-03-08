// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelActorBase.h"
#include "VoxelLandscapeChunkKey.h"

class AVoxelLandscape;
class FVoxelLandscapeState;
class UVoxelLandscapeNaniteComponent;
class UVoxelLandscapeHeightComponent;
class UVoxelLandscapeVolumeComponent;

class VOXELLANDSCAPE_API FVoxelLandscapeRuntime : public IVoxelActorRuntime
{
public:
	const TWeakObjectPtr<AVoxelLandscape> WeakLandscape;

	explicit FVoxelLandscapeRuntime(AVoxelLandscape& Landscape);

	void Initialize();

	//~ Begin IVoxelActorRuntime Interface
	virtual void Tick() override;
	//~ End IVoxelActorRuntime Interface

private:
	bool bIsInvalidated = true;
	TOptional<FVoxelBox2D> InvalidatedHeightBounds;
	TOptional<FVoxelBox> InvalidatedVolumeBounds;

	TSharedPtr<FVoxelLandscapeState> NewState;
	TSharedPtr<FVoxelLandscapeState> OldState;
	TVoxelMap<FVoxelLandscapeChunkKey, TWeakObjectPtr<UVoxelLandscapeNaniteComponent>> ChunkKeyToNaniteComponent;
	TVoxelMap<FVoxelLandscapeChunkKey, TWeakObjectPtr<UVoxelLandscapeHeightComponent>> ChunkKeyToHeightComponent;
	TVoxelMap<FVoxelLandscapeChunkKey, TWeakObjectPtr<UVoxelLandscapeVolumeComponent>> ChunkKeyToVolumeComponent;

	void Render(const FVoxelLandscapeState& State);
};