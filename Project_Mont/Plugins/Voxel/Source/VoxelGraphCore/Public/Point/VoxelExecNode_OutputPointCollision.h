// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "Buffer/VoxelNameBuffer.h"
#include "Point/VoxelChunkedPointSet.h"
#include "VoxelExecNode_OutputPointCollision.generated.h"

class FVoxelInvokerView;
class FVoxelPointCollisionLargeChunk;
class FVoxelPointCollisionSmallChunk;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelExecNode_OutputPointCollision : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelChunkedPointSet, ChunkedPoints, nullptr, ConstantPin);
	VOXEL_INPUT_PIN(FBodyInstance, BodyInstance, nullptr, ConstantPin);
	VOXEL_INPUT_PIN(FName, InvokerChannel, "Default", ConstantPin);
	// Will be added to invoker radius
	VOXEL_INPUT_PIN(float, DistanceOffset, 0.f, ConstantPin);
	// In cm, granularity of collision
	// Try to keep high enough to not have too many chunks
	VOXEL_INPUT_PIN(int32, ChunkSize, 1000, ConstantPin);
	// These attributes will be free to cache if collision is computed for them
	VOXEL_INPUT_PIN(FVoxelNameBuffer, AttributesToCache, nullptr, ArrayPin, ConstantPin);
	// If true, this component will generate overlap events when it is overlapping other components (eg Begin Overlap).
	// Both components (this and the other) must have this enabled for overlap events to occur.
	VOXEL_INPUT_PIN(bool, GenerateOverlapEvents, false, ConstantPin, AdvancedDisplay);
	// If true, this component will generate individual overlaps for each overlapping physics body
	VOXEL_INPUT_PIN(bool, MultiBodyOverlap, true, ConstantPin, AdvancedDisplay);
	// Priority offset, added to the task distance from camera
	// Closest tasks are computed first, so set this to a very low value (eg, -1000000) if you want it to be computed first
	VOXEL_INPUT_PIN(double, PriorityOffset, -1000000, ConstantPin, AdvancedDisplay);

	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_OutputPointCollision : public TVoxelExecNodeRuntime<FVoxelExecNode_OutputPointCollision>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	virtual void Destroy() override;
	//~ End FVoxelExecNodeRuntime Interface

private:
	TSharedPtr<FVoxelInvokerView> InvokerView;

	mutable FVoxelCriticalSection CriticalSection;
	TVoxelMap<FIntVector, TWeakPtr<FVoxelPointCollisionLargeChunk>> LargeChunks_RequiresLock;
	TVoxelMap<FIntVector, TSharedPtr<FVoxelPointCollisionSmallChunk>> SmallChunks_RequiresLock;
};