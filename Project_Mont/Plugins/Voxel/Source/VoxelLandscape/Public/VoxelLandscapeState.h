// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeChunkKey.h"

class AVoxelLandscape;
class FVoxelLandscapeBrushes;
class FVoxelLandscapeNaniteMesh;
class FVoxelLandscapeHeightMesh;
class FVoxelLandscapeVolumeMesh;

class VOXELLANDSCAPE_API FVoxelLandscapeState : public TSharedFromThis<FVoxelLandscapeState>
{
public:
	const FObjectKey World;
	const FVector CameraPosition;

	const FMatrix VoxelToWorld;
	const FMatrix WorldToVoxel;
	const FTransform2d VoxelToWorld2D;
	const FTransform2d WorldToVoxel2D;

	const int32 VoxelSize;
	const int32 ChunkSize;
	const double TargetVoxelSizeOnScreen;
	const int32 DetailTextureSize;
	const bool bEnableNanite;

	const TSharedRef<FVoxelMaterialRef> Material;

	TSharedPtr<const FVoxelLandscapeState> LastState;
	const TOptional<FVoxelBox2D> InvalidatedHeightBounds;
	const TOptional<FVoxelBox> InvalidatedVolumeBounds;

	FVoxelLandscapeState(
		const AVoxelLandscape& Actor,
		const FVector& CameraPosition,
		const TSharedPtr<const FVoxelLandscapeState>& LastState,
		const TOptional<FVoxelBox2D>& InvalidatedHeightBounds,
		const TOptional<FVoxelBox>& InvalidatedVolumeBounds);

	void Compute();

public:
	FORCEINLINE bool IsReadyToRender() const
	{
		return bIsReadyToRender.Get();
	}
	FORCEINLINE const FVoxelLandscapeBrushes& GetBrushes() const
	{
		checkVoxelSlow(Brushes.IsValid());
		return *Brushes.Get();
	}

	FORCEINLINE const TVoxelMap<FVoxelLandscapeChunkKey, TSharedPtr<FVoxelLandscapeNaniteMesh>>& GetChunkKeyToNaniteMesh() const
	{
		return ChunkKeyToNaniteMesh;
	}
	FORCEINLINE const TVoxelMap<FVoxelLandscapeChunkKey, TSharedPtr<FVoxelLandscapeHeightMesh>>& GetChunkKeyToHeightMesh() const
	{
		return ChunkKeyToHeightMesh;
	}
	FORCEINLINE const TVoxelMap<FVoxelLandscapeChunkKey, TSharedPtr<FVoxelLandscapeVolumeMesh>>& GetChunkKeyToVolumeMesh() const
	{
		return ChunkKeyToVolumeMesh;
	}

private:
	TVoxelAtomic<bool> bIsReadyToRender;
	TSharedPtr<FVoxelLandscapeBrushes> Brushes;
	TVoxelSet<FVoxelLandscapeChunkKey> EmptyChunks;
	TVoxelMap<FVoxelLandscapeChunkKey, TSharedPtr<FVoxelLandscapeNaniteMesh>> ChunkKeyToNaniteMesh;
	TVoxelMap<FVoxelLandscapeChunkKey, TSharedPtr<FVoxelLandscapeHeightMesh>> ChunkKeyToHeightMesh;
	TVoxelMap<FVoxelLandscapeChunkKey, TSharedPtr<FVoxelLandscapeVolumeMesh>> ChunkKeyToVolumeMesh;
};