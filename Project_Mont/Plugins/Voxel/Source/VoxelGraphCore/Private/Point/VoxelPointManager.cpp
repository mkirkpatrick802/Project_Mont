// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelPointManager.h"

TSharedRef<FVoxelPointOverrideChunk> FVoxelPointManager::FindOrAddChunk(const FVoxelPointChunkRef& ChunkRef)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	TSharedPtr<FVoxelPointOverrideChunk>& ChunkData = ChunkRefToChunk_RequiresLock.FindOrAdd(ChunkRef);
	if (!ChunkData)
	{
		ChunkData = MakeVoxelShared<FVoxelPointOverrideChunk>();
	}
	return ChunkData.ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimePinValue FVoxelPointManager::FindAttribute(
	const FName AttributeName, 
	const FVoxelPointChunkRef& ChunkRef,
	const FVoxelPointId& PointId) const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	const TVoxelMap<FVoxelPointChunkRef, TVoxelArray<FFindPoint>>* ChunkRefToFindPointsPtr = AttributeNameToChunkRefToFindPoints_RequiresLock.Find(AttributeName);
	if (!ChunkRefToFindPointsPtr)
	{
		return {};
	}

	const TVoxelArray<FFindPoint>* FindPointsPointer = ChunkRefToFindPointsPtr->Find(ChunkRef);
	if (!FindPointsPointer)
	{
		return {};
	}

	for (const FFindPoint& FindPoint : *FindPointsPointer)
	{
		FVoxelRuntimePinValue Value;
		if (!FindPoint.ExecuteIfBound(PointId, Value) ||
			!Value.IsValid())
		{
			continue;
		}

		return Value;
	}
	return {};
}

void FVoxelPointManager::RegisterFindPoint(
	const FName AttributeName, 
	const FVoxelPointChunkRef& ChunkRef,
	const FFindPoint& FindPoint)
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	AttributeNameToChunkRefToFindPoints_RequiresLock.FindOrAdd(AttributeName).FindOrAdd(ChunkRef).Add(FindPoint);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelPointManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	for (auto It = ChunkRefToChunk_RequiresLock.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsUnique())
		{
			continue;
		}

		{
			VOXEL_SCOPE_LOCK(It.Value()->CriticalSection);

			if (It.Value()->PointIdsToHide_RequiresLock.Num() > 0)
			{
				continue;
			}
		}

		// Unique and nothing stored in it: safe to remove
		It.RemoveCurrent();
	}

	for (auto AttributeIt = AttributeNameToChunkRefToFindPoints_RequiresLock.CreateIterator(); AttributeIt; ++AttributeIt)
	{
		for (auto ChunkIt = AttributeIt.Value().CreateIterator(); ChunkIt; ++ChunkIt)
		{
			ChunkIt.Value().RemoveAllSwap([&](const FFindPoint& FindPoint)
			{
				return !FindPoint.IsBound();
			});

			if (ChunkIt.Value().Num() == 0)
			{
				ChunkIt.RemoveCurrent();
			}
		}

		if (AttributeIt.Value().Num() == 0)
		{
			AttributeIt.RemoveCurrent();
		}
	}
}