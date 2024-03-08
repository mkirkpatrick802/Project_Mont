// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelExecNode_OutputPointMesh.h"
#include "VoxelRenderMeshChunk.h"
#include "VoxelInvoker.h"

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_OutputPointMesh::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_OutputPointMesh>(SharedThis);
}

void FVoxelExecNodeRuntime_OutputPointMesh::Create()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Chunks_RequiresLock.Num() == 0);

	const TSharedRef<const FVoxelChunkedPointSet> ChunkedPoints = GetConstantPin(Node.ChunkedPointsPin);
	const float RenderDistance = GetConstantPin(Node.RenderDistancePin);
	const float MinRenderDistance = GetConstantPin(Node.MinRenderDistancePin);
	const float FadeDistance = GetConstantPin(Node.FadeDistancePin);
	const TSharedRef<const FVoxelInstancedMeshComponentSettings> MeshSettings = GetConstantPin(Node.FoliageSettingsPin);

	if (!ChunkedPoints->IsValid())
	{
		return;
	}

	const int32 ChunkSize = ChunkedPoints->GetChunkSize();

	InvokerView = FVoxelInvokerManager::Get(GetWorld())->MakeView(
		STATIC_FNAME("Camera"),
		ChunkSize,
		FMath::CeilToInt(GetConstantPin(Node.RenderDistancePin)),
		GetLocalToWorld());

	InvokerView->Bind_Async(
		MakeWeakPtrDelegate(this, [=](const TVoxelSet<FIntVector>& ChunksToAdd)
		{
			VOXEL_SCOPE_COUNTER("OnAddChunk");

			TVoxelArray<TWeakPtr<FVoxelRenderMeshChunk>> WeakChunks;
			WeakChunks.Reserve(ChunksToAdd.Num());
			{
				VOXEL_SCOPE_LOCK(CriticalSection);

				for (const FIntVector& ChunkKey : ChunksToAdd)
				{
					const FIntVector ChunkMin = ChunkKey * ChunkSize;

					FVoxelPointChunkRef ChunkRef;
					ChunkRef.ChunkProviderRef = ChunkedPoints->GetChunkProviderRef();
					ChunkRef.ChunkMin = ChunkMin;
					ChunkRef.ChunkSize = ChunkSize;

					const TSharedRef<FVoxelRenderMeshChunk> Chunk = MakeVoxelShared<FVoxelRenderMeshChunk>(
						GetNodeRef(),
						ChunkRef,
						GetRuntimeInfo(),
						ChunkedPoints->GetPointsValue(ChunkMin, GetConstantPin(Node.PriorityOffsetPin)),
						RenderDistance,
						MinRenderDistance,
						FadeDistance,
						MeshSettings);

					Chunk->Initialize();

					WeakChunks.Add(Chunk);

					Chunks_RequiresLock.Add_EnsureNew(ChunkMin, Chunk);
				}
			}
		}),
		MakeWeakPtrDelegate(this, [=](const TVoxelSet<FIntVector>& ChunksToRemove)
		{
			VOXEL_SCOPE_COUNTER("OnRemoveChunk");

			TVoxelArray<TSharedPtr<FVoxelRenderMeshChunk>> ChunksToDestroy;
			{
				VOXEL_SCOPE_LOCK(CriticalSection);
				for (const FIntVector& ChunkKey : ChunksToRemove)
				{
					const FIntVector ChunkMin = ChunkKey * ChunkSize;

					TSharedPtr<FVoxelRenderMeshChunk> Chunk;
					if (!ensure(Chunks_RequiresLock.RemoveAndCopyValue(ChunkMin, Chunk)))
					{
						continue;
					}

					ChunksToDestroy.Add(Chunk);
				}
			}

			RunOnGameThread(MakeWeakPtrLambda(this, [this, ChunksToDestroy = MoveTemp(ChunksToDestroy)]
			{
				VOXEL_SCOPE_COUNTER("OnRemoveChunk_GameThread");

				const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
				if (!ensure(Runtime))
				{
					return;
				}

				for (const TSharedPtr<FVoxelRenderMeshChunk>& Chunk : ChunksToDestroy)
				{
					Chunk->Destroy(*Runtime);
				}
			}));
		}));
}

void FVoxelExecNodeRuntime_OutputPointMesh::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	InvokerView = {};

	VOXEL_SCOPE_LOCK(CriticalSection);

	const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
	if (!Runtime)
	{
		return;
	}

	for (const auto& It : Chunks_RequiresLock)
	{
		It.Value->Destroy(*Runtime);
	}
}

FVoxelOptionalBox FVoxelExecNodeRuntime_OutputPointMesh::GetBounds() const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	FVoxelOptionalBox Bounds;
	for (const auto& It : Chunks_RequiresLock)
	{
		Bounds += It.Value->GetBounds();
	}
	return Bounds;
}