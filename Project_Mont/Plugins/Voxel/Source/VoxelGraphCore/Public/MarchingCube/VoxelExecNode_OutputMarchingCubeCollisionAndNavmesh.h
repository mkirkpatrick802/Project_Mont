// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSurface.h"
#include "VoxelExecNode.h"
#include "VoxelFastOctree.h"
#include "VoxelPhysicalMaterial.h"
#include "VoxelExecNode_OutputMarchingCubeCollisionAndNavmesh.generated.h"

class FVoxelInvokerView;
class FVoxelMarchingCubeCollider;
class UVoxelMarchingCubeNavmeshComponent;
class UVoxelMarchingCubeCollisionComponent;
struct FVoxelMarchingCubeColliderWrapper;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelExecNode_OutputMarchingCubeCollisionAndNavmesh : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr, VirtualPin);
	VOXEL_INPUT_PIN(FBodyInstance, BodyInstance, nullptr, ConstantPin);
	VOXEL_INPUT_PIN(FName, InvokerChannel, "Default", ConstantPin);
	VOXEL_INPUT_PIN(int32, VoxelSize, 100, ConstantPin);
	VOXEL_INPUT_PIN(bool, ComputeCollision, true, ConstantPin);
	VOXEL_INPUT_PIN(bool, ComputeNavmesh, false, ConstantPin);

	VOXEL_INPUT_PIN(FVoxelPhysicalMaterialBuffer, PhysicalMaterial, nullptr, VirtualPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(float, DistanceChecksTolerance, 1.f, VirtualPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(int32, ChunkSize, 32, ConstantPin, AdvancedDisplay);
	// Priority offset, added to the task distance from camera
	// Closest tasks are computed first, so set this to a very low value (eg, -1000000) if you want it to be computed first
	VOXEL_INPUT_PIN(double, PriorityOffset, -2000000, ConstantPin, AdvancedDisplay);

	TValue<FVoxelMarchingCubeColliderWrapper> CreateCollider(
		const FVoxelQuery& InQuery,
		int32 VoxelSize,
		int32 ChunkSize,
		const FVoxelBox& Bounds) const;
	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_OutputMarchingCubeCollisionAndNavmesh : public TVoxelExecNodeRuntime<FVoxelExecNode_OutputMarchingCubeCollisionAndNavmesh>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	virtual void Destroy() override;
	virtual void Tick(FVoxelRuntime& Runtime) override;
	//~ End FVoxelExecNodeRuntime Interface

private:
	struct FChunk
	{
		FChunk() = default;
		~FChunk()
		{
			ensure(!Collider_RequiresLock.IsValid());
			ensure(!CollisionComponent_GameThread.IsValid());
			ensure(!NavmeshComponent_GameThread.IsValid());
		}

		TVoxelDynamicValue<FVoxelMarchingCubeColliderWrapper> Collider_RequiresLock;
		TWeakObjectPtr<UVoxelMarchingCubeCollisionComponent> CollisionComponent_GameThread;
		TWeakObjectPtr<UVoxelMarchingCubeNavmeshComponent> NavmeshComponent_GameThread;
	};

public:
	TSharedPtr<const FBodyInstance> BodyInstance;
	bool bComputeCollision = false;
	bool bComputeNavmesh = false;
	TSharedPtr<FVoxelInvokerView> InvokerView;

	FVoxelCriticalSection CriticalSection;
	TVoxelMap<FIntVector, TSharedPtr<FChunk>> Chunks_RequiresLock;

	struct FQueuedCollider
	{
		TWeakPtr<FChunk> Chunk;
		TSharedPtr<const FVoxelMarchingCubeCollider> Collider;
	};
	TQueue<FQueuedCollider, EQueueMode::Mpsc> QueuedColliders;
	TQueue<TSharedPtr<FChunk>, EQueueMode::Mpsc> ChunksToDestroy;

	void ProcessChunksToDestroy();
	void ProcessQueuedColliders(FVoxelRuntime& Runtime);
};