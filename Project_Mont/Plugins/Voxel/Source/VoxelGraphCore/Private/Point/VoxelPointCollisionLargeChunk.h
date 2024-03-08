// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNodeInterface.h"
#include "Buffer/VoxelNameBuffer.h"
#include "Point/VoxelChunkedPointSet.h"
#include "Buffer/VoxelStaticMeshBuffer.h"

class FVoxelInstancedCollisionData;
class UVoxelInstancedCollisionComponent;

class FVoxelPointCollisionLargeChunk
	: public IVoxelNodeInterface
	, public TSharedFromThis<FVoxelPointCollisionLargeChunk>
{
public:
	const FVoxelGraphNodeRef NodeRef;
	const FVoxelPointChunkRef ChunkRef;
	const TVoxelDynamicValue<FVoxelPointSet> PointsValue;
	const FVoxelNameBuffer AttributesToCache;

	VOXEL_COUNT_INSTANCES();

	FVoxelPointCollisionLargeChunk(
		const FVoxelGraphNodeRef& NodeRef,
		const FVoxelPointChunkRef& ChunkRef,
		const TVoxelDynamicValue<FVoxelPointSet>& PointsValue,
		const FVoxelNameBuffer& AttributesToCache);

	void Initialize(const FObjectKey& World);

public:
	struct FPointTransform
	{
		float PositionX;
		float PositionY;
		float PositionZ;
		float ScaleX;
		float ScaleY;
		float ScaleZ;
		float RotationX;
		float RotationY;
		float RotationZ;
		float RotationW;
	};

	using FMeshToPoints = TVoxelMap<FVoxelStaticMesh, TVoxelMap<FVoxelPointId, FPointTransform>>;
	using FOnChanged = TFunction<void(const FMeshToPoints& MeshToPoints)>;

	void AddOnChanged(const FOnChanged& OnChanged);

	//~ Begin IVoxelNodeInterface Interface
	virtual const FVoxelGraphNodeRef& GetNodeRef() const override
	{
		return NodeRef;
	}
	//~ End IVoxelNodeInterface Interface

private:
	mutable FVoxelCriticalSection CriticalSection;

	TSharedPtr<const FMeshToPoints> MeshToPoints_RequiresLock;
	TVoxelArray<FOnChanged> OnChanged_RequiresLock;

	TVoxelMap<FVoxelPointId, int32> CachedAttributeIds_RequiresLock;
	TVoxelMap<FName, TSharedPtr<const FVoxelBuffer>> CachedAttributes_RequiresLock;

	void UpdatePoints(const TSharedRef<const FVoxelPointSet>& NewPoints);
};