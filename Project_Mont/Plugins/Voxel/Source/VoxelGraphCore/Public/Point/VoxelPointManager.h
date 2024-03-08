// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Point/VoxelPointHandle.h"

class VOXELGRAPHCORE_API FVoxelPointOverrideChunk
{
public:
	FVoxelCriticalSection CriticalSection;
	TMulticastDelegate<void(TConstVoxelArrayView<FVoxelPointId> PointIds)> OnChanged_RequiresLock;
	TVoxelSet<FVoxelPointId> PointIdsToHide_RequiresLock;
};

class VOXELGRAPHCORE_API FVoxelPointManager : public IVoxelWorldSubsystem
{
public:
	GENERATED_VOXEL_WORLD_SUBSYSTEM_BODY(FVoxelPointManager);

	TSharedRef<FVoxelPointOverrideChunk> FindOrAddChunk(const FVoxelPointChunkRef& ChunkRef);

public:
	using FFindPoint = TDelegate<void(FVoxelPointId PointId, FVoxelRuntimePinValue& OutValue)>;

	FVoxelRuntimePinValue FindAttribute(
		FName AttributeName,
		const FVoxelPointChunkRef& ChunkRef,
		const FVoxelPointId& PointId) const;

	void RegisterFindPoint(
		FName AttributeName,
		const FVoxelPointChunkRef& ChunkRef,
		const FFindPoint& FindPoint);

public:
	//~ Begin IVoxelWorldSubsystem Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End IVoxelWorldSubsystem Interface

private:
	mutable FVoxelCriticalSection CriticalSection;
	TVoxelMap<FVoxelPointChunkRef, TSharedPtr<FVoxelPointOverrideChunk>> ChunkRefToChunk_RequiresLock;
	TVoxelMap<FName, TVoxelMap<FVoxelPointChunkRef, TVoxelArray<FFindPoint>>> AttributeNameToChunkRefToFindPoints_RequiresLock;
};