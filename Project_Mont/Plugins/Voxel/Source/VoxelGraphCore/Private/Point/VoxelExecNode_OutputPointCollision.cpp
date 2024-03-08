// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelExecNode_OutputPointCollision.h"
#include "VoxelPointCollisionLargeChunk.h"
#include "VoxelPointCollisionSmallChunk.h"
#include "VoxelInvoker.h"

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_OutputPointCollision::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_OutputPointCollision>(SharedThis);
}

void FVoxelExecNodeRuntime_OutputPointCollision::Create()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(SmallChunks_RequiresLock.Num() == 0);
	ensure(LargeChunks_RequiresLock.Num() == 0);

	const TSharedRef<const FVoxelChunkedPointSet> ChunkedPoints = GetConstantPin(Node.ChunkedPointsPin);

	if (!ChunkedPoints->IsValid())
	{
		return;
	}

	const int32 LargeChunkSize = ChunkedPoints->GetChunkSize();
	const int32 SmallChunkSize = GetConstantPin(Node.ChunkSizePin);

	InvokerView = FVoxelInvokerManager::Get(GetWorld())->MakeView(
			GetConstantPin(Node.InvokerChannelPin),
			SmallChunkSize,
			FMath::CeilToInt(GetConstantPin(Node.DistanceOffsetPin)),
			GetLocalToWorld());

	InvokerView->Bind_Async(
		MakeWeakPtrDelegate(this, [=](const TVoxelSet<FIntVector>& ChunksToAdd)
		{
			VOXEL_SCOPE_COUNTER("OnAddChunk");
			VOXEL_SCOPE_LOCK(CriticalSection);

			for (const FIntVector& ChunkKey : ChunksToAdd)
			{
				const FIntVector SmallChunkMin = ChunkKey * SmallChunkSize;
				const FIntVector LargeChunkMin = FVoxelUtilities::DivideFloor(SmallChunkMin, LargeChunkSize) * LargeChunkSize;

				FVoxelPointChunkRef ChunkRef;
				ChunkRef.ChunkProviderRef = ChunkedPoints->GetChunkProviderRef();
				ChunkRef.ChunkMin = LargeChunkMin;
				ChunkRef.ChunkSize = LargeChunkSize;

				TWeakPtr<FVoxelPointCollisionLargeChunk>& WeakLargeChunk = LargeChunks_RequiresLock.FindOrAdd(LargeChunkMin);
				TSharedPtr<FVoxelPointCollisionLargeChunk> LargeChunk = WeakLargeChunk.Pin();
				if (!LargeChunk)
				{
					LargeChunk = MakeVoxelShared<FVoxelPointCollisionLargeChunk>(
						GetNodeRef(),
						ChunkRef,
						ChunkedPoints->GetPointsValue(LargeChunkMin, GetConstantPin(Node.PriorityOffsetPin)),
						GetConstantPin(Node.AttributesToCachePin));
					LargeChunk->Initialize(GetWorld());

					WeakLargeChunk = LargeChunk;
				}

				const TSharedRef<FVoxelPointCollisionSmallChunk> Chunk = MakeVoxelShared<FVoxelPointCollisionSmallChunk>(
					GetNodeRef(),
					ChunkRef,
					FVoxelBox(SmallChunkMin, SmallChunkMin + SmallChunkSize),
					GetRuntimeInfo(),
					GetConstantPin(Node.BodyInstancePin),
					GetConstantPin(Node.GenerateOverlapEventsPin),
					GetConstantPin(Node.MultiBodyOverlapPin),
					LargeChunk.ToSharedRef());

				Chunk->Initialize();

				SmallChunks_RequiresLock.Add_EnsureNew(SmallChunkMin, Chunk);
			}
		}),
		MakeWeakPtrDelegate(this, [=](const TVoxelSet<FIntVector>& ChunksToRemove)
		{
			VOXEL_SCOPE_COUNTER("OnRemoveChunk");

			TVoxelArray<TSharedPtr<FVoxelPointCollisionSmallChunk>> ChunksToDestroy;
			{
				VOXEL_SCOPE_LOCK(CriticalSection);
				for (const FIntVector& ChunkKey : ChunksToRemove)
				{
					const FIntVector ChunkMin = ChunkKey * SmallChunkSize;

					TSharedPtr<FVoxelPointCollisionSmallChunk> Chunk;
					if (!ensure(SmallChunks_RequiresLock.RemoveAndCopyValue(ChunkMin, Chunk)))
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

				for (const TSharedPtr<FVoxelPointCollisionSmallChunk>& Chunk : ChunksToDestroy)
				{
					Chunk->Destroy(*Runtime);
				}
			}));
		}));
}

void FVoxelExecNodeRuntime_OutputPointCollision::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	InvokerView = {};

	const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
	if (!Runtime)
	{
		return;
	}

	VOXEL_SCOPE_LOCK(CriticalSection);
	for (const auto& It : SmallChunks_RequiresLock)
	{
		It.Value->Destroy(*Runtime);
	}
	SmallChunks_RequiresLock.Empty();
}