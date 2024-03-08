// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurface.h"
#include "VoxelFastOctree.h"
#include "VoxelChunkSpawner.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelInvokerChunkSpawner.generated.h"

class FVoxelInvokerView;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelInvokerChunkSpawner : public FVoxelChunkSpawner
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelGraphNodeRef NodeRef;
	int32 LOD = 0;
	float WorldSize = 0.f;
	FName InvokerChannel;
	int32 ChunkSize = 0;
	int32 GroupSize = 0.f;

	//~ Begin FVoxelChunkSpawnerImpl Interface
	virtual void Initialize(const FVoxelExecNodeRuntime_OutputMarchingCubeMesh& NodeRuntime) override;
	virtual void Tick(const FVoxelExecNodeRuntime_OutputMarchingCubeMesh& NodeRuntime) override;
	//~ End FVoxelChunkSpawnerImpl Interface

private:
	int32 ScaledChunkSize = 0;
	TSharedPtr<FVoxelTaskReferencer> Referencer;
	TSharedPtr<FVoxelTerminalGraphInstance> TerminalGraphInstance;
	TSharedPtr<const TVoxelComputeValue<FVoxelSurface>> ComputeDistanceChecksSurface;
	TSharedPtr<const TVoxelComputeValue<float>> ComputeDistanceChecksTolerance;

	TSharedPtr<FVoxelInvokerView> InvokerView_GameThread;
	FSharedVoidPtr InvokerViewBindRef_GameThread;

	struct FBulkChunk
	{
		FVoxelCriticalSection CriticalSection;
		TSharedPtr<FVoxelTaskGroup> TaskGroup_RequiresLock;
		TSharedPtr<FVoxelDependencyTracker> DependencyTracker_RequiresLock;
		TVoxelMap<FIntVector, TSharedPtr<FVoxelChunkRef>> ChunkRefs_RequiresLock;
	};
	FVoxelCriticalSection CriticalSection;
	TVoxelMap<FIntVector, TSharedPtr<FBulkChunk>> BulkChunkRefs_RequiresLock;

	void UpdateBulkChunk(
		const FIntVector& Chunk,
		const TSharedRef<FBulkChunk>& InBulkChunk) const;
};

USTRUCT(Category = "Chunk Spawner")
struct VOXELGRAPHCORE_API FVoxelNode_MakeInvokerChunkSpawner : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(int32, LOD, 0);
	VOXEL_INPUT_PIN(float, WorldSize, 1.e6f);
	VOXEL_INPUT_PIN(FName, InvokerChannel, "Default");
	VOXEL_INPUT_PIN(int32, ChunkSize, 32, AdvancedDisplay);
	// Distance checks will be done for chunks in bulk
	// Bulk query size (actual amount will be cubed)
	// Will also affect render granularity, if too many chunks are spawned at once reduce this
	VOXEL_INPUT_PIN(int32, GroupSize, 4, AdvancedDisplay);

	VOXEL_OUTPUT_PIN(FVoxelChunkSpawner, Spawner);
};