// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelAABBTree.h"
#include "VoxelAABBTree2D.h"

class FVoxelLandscapeHeightBrushRuntime;
class FVoxelLandscapeVolumeBrushRuntime;

class FVoxelLandscapeBrushes
{
public:
	FVoxelLandscapeBrushes(
		const FMatrix& WorldToVoxel,
		const TVoxelSparseArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>>& InHeightBrushes,
		const TVoxelSparseArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>>& InVolumeBrushes);

	bool IsEmpty() const;
	FVoxelBox GetBounds() const;
	bool Intersect(const FVoxelBox& Bounds) const;
	bool HasVolumeBrushes(const FVoxelBox& Bounds) const;

	TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> GetHeightBrushes(const FVoxelBox2D& Bounds) const;
	TVoxelArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> GetVolumeBrushes(const FVoxelBox& Bounds) const;

private:
	struct FPayload
	{
		uint32 bIsHeight : 1;
		uint32 Index : 31;

		FORCEINLINE FPayload()
		{
			ReinterpretCastRef<uint32>(*this) = 0;
		}
	};

	FVoxelAABBTree2D HeightTree;
	FVoxelAABBTree VolumeTree;
	TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> HeightBrushes;
	TVoxelArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> VolumeBrushes;
};

class VOXELLANDSCAPE_API FVoxelLandscapeBrushManager : public IVoxelWorldSubsystem
{
public:
	GENERATED_VOXEL_WORLD_SUBSYSTEM_BODY(FVoxelLandscapeBrushManager);

	TMulticastDelegate<void(const FVoxelLandscapeVolumeBrushRuntime&)> OnChanged_GameThread;
	TMulticastDelegate<void(const FVoxelLandscapeHeightBrushRuntime&)> OnChanged2D_GameThread;

	int32 RegisterHeightBrush(const TSharedRef<FVoxelLandscapeHeightBrushRuntime>& Brush);
	void UnregisterHeightBrush(int32 BrushIndex);

	int32 RegisterVolumeBrush(const TSharedRef<FVoxelLandscapeVolumeBrushRuntime>& Brush);
	void UnregisterVolumeBrush(int32 BrushIndex);

	TVoxelFuture<FVoxelLandscapeBrushes> GetBrushes(const FMatrix& WorldToVoxel);

public:
	//~ Begin IVoxelWorldSubsystem Interface
	virtual void Tick() override;
	//~ End IVoxelWorldSubsystem Interface

private:
	TVoxelSparseArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> HeightBrushes_GameThread;
	TVoxelSparseArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> VolumeBrushes_GameThread;

private:
	enum class EType : uint8
	{
		AddHeight,
		AddVolume,
		RemoveHeight,
		RemoveVolume
	};
	struct FAction
	{
		EType Type = {};
		int32 BrushIndex = -1;
		FSharedVoidPtr Brush;
	};
	TVoxelArray<FAction> PendingActions_GameThread;

	struct FAsyncData
	{
		FVoxelCriticalSection CriticalSection;
		TVoxelSparseArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> HeightBrushes;
		TVoxelSparseArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> VolumeBrushes;
	};
	FAsyncData AsyncData;
};