// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "MarchingCube/VoxelExecNode_OutputMarchingCubeMesh.h"

class UVoxelMarchingCubeCollisionComponent;

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_OutputMarchingCubeMesh : public TVoxelExecNodeRuntime<FVoxelExecNode_OutputMarchingCubeMesh>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	virtual void Destroy() override;
	virtual void Tick(FVoxelRuntime& Runtime) override;
	//~ End FVoxelExecNodeRuntime Interface

private:
	const TSharedRef<FVoxelChunkActionQueue> ChunkActionQueue = MakeVoxelShared<FVoxelChunkActionQueue>();

	TSharedPtr<FVoxelChunkSpawner> ChunkSpawner;
	int32 VoxelSize = 0;

	struct FChunkInfo
	{
		const FVoxelChunkId ChunkId = FVoxelChunkId::New();
		const int32 LOD;
		const int32 ChunkSize;
		const FVoxelBox Bounds;

		FChunkInfo(
			const int32 LOD,
			const int32 ChunkSize,
			const FVoxelBox& Bounds)
			: LOD(LOD)
			, ChunkSize(ChunkSize)
			, Bounds(Bounds)
		{
		}
		~FChunkInfo()
		{
			ensure(!Mesh.IsValid());
			ensure(!MeshComponent.IsValid());
			ensure(!CollisionComponent.IsValid());
			ensure(OnCompleteArray.Num() == 0);
		}

		TVoxelDynamicValue<FVoxelMarchingCubeExecNodeMesh> Mesh;
		uint8 TransitionMask = 0;
		TWeakObjectPtr<UVoxelMarchingCubeMeshComponent> MeshComponent;
		TWeakObjectPtr<UVoxelMarchingCubeCollisionComponent> CollisionComponent;
		TVoxelArray<TSharedPtr<const TVoxelUniqueFunction<void()>>> OnCompleteArray;

		void FlushOnComplete();
	};

	FVoxelCriticalSection ChunkInfos_CriticalSection;
	TMap<FVoxelChunkId, TSharedPtr<FChunkInfo>> ChunkInfos;

	struct FQueuedMesh
	{
		FVoxelChunkId ChunkId;
		TSharedPtr<const FVoxelMarchingCubeExecNodeMesh> Mesh;
	};
	using FQueuedMeshes = TQueue<FQueuedMesh, EQueueMode::Mpsc>;
	const TSharedRef<FQueuedMeshes> QueuedMeshes = MakeVoxelShared<FQueuedMeshes>();

	FGraphEventRef ProcessActionsGraphEvent;

	void ProcessMeshes(FVoxelRuntime& Runtime);
	void ProcessActions(FVoxelRuntime* Runtime, bool bIsInGameThread);
	void ProcessAction(FVoxelRuntime* Runtime, const FVoxelChunkAction& Action);
};