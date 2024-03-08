// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelExecNode_OutputMarchingCubeCollisionAndNavmesh.h"
#include "MarchingCube/VoxelMarchingCubeNavmesh.h"
#include "MarchingCube/VoxelMarchingCubeNavmeshComponent.h"
#include "MarchingCube/VoxelMarchingCubeCollisionComponent.h"
#include "MarchingCube/VoxelNode_CreateMarchingCubeCollider.h"
#include "MarchingCube/VoxelNode_GenerateMarchingCubeSurface.h"
#include "VoxelInvoker.h"
#include "VoxelRuntime.h"
#include "Chaos/TriangleMeshImplicitObject.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelCollisionEnableInEditor, false,
	"voxel.collision.EnableInEditor",
	"");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFutureValue<FVoxelMarchingCubeColliderWrapper> FVoxelExecNode_OutputMarchingCubeCollisionAndNavmesh::CreateCollider(
	const FVoxelQuery& InQuery,
	const int32 VoxelSize,
	const int32 ChunkSize,
	const FVoxelBox& Bounds) const
{
	checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));
	const FVoxelQuery Query = InQuery.EnterScope(*this);

	return VOXEL_CALL_NODE(FVoxelNode_CreateMarchingCubeCollider, ColliderPin, Query)
	{
		VOXEL_CALL_NODE_BIND(SurfacePin, VoxelSize, ChunkSize, Bounds)
		{
			return VOXEL_CALL_NODE(FVoxelNode_GenerateMarchingCubeSurface, SurfacePin, Query)
			{
				VOXEL_CALL_NODE_BIND(DistancePin)
				{
					const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
					Parameters->Add<FVoxelSurfaceQueryParameter>().ComputeDistance();
					const TValue<FVoxelSurface> Surface = GetNodeRuntime().Get(SurfacePin, Query.MakeNewQuery(Parameters));

					return VOXEL_ON_COMPLETE(Surface)
					{
						return Surface->GetDistance();
					};
				};
				VOXEL_CALL_NODE_BIND(VoxelSizePin, VoxelSize)
				{
					return VoxelSize;
				};
				VOXEL_CALL_NODE_BIND(ChunkSizePin, ChunkSize)
				{
					return ChunkSize;
				};
				VOXEL_CALL_NODE_BIND(BoundsPin, Bounds)
				{
					return Bounds;
				};
				VOXEL_CALL_NODE_BIND(EnableTransitionsPin)
				{
					return false;
				};
				VOXEL_CALL_NODE_BIND(PerfectTransitionsPin)
				{
					return false;
				};
				VOXEL_CALL_NODE_BIND(EnableDistanceChecksPin)
				{
					return true;
				};
				VOXEL_CALL_NODE_BIND(DistanceChecksTolerancePin)
				{
					return GetNodeRuntime().Get(DistanceChecksTolerancePin, Query);
				};
			};
		};

		VOXEL_CALL_NODE_BIND(PhysicalMaterialPin)
		{
			return GetNodeRuntime().Get(PhysicalMaterialPin, Query);
		};
	};
}

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_OutputMarchingCubeCollisionAndNavmesh::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_OutputMarchingCubeCollisionAndNavmesh>(SharedThis);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_OutputMarchingCubeCollisionAndNavmesh::Create()
{
	VOXEL_FUNCTION_COUNTER();

	if (!IsGameWorld() &&
		!GVoxelCollisionEnableInEditor)
	{
		return;
	}

	BodyInstance = GetConstantPin(Node.BodyInstancePin);
	bComputeCollision = GetConstantPin(Node.ComputeCollisionPin);
	bComputeNavmesh = GetConstantPin(Node.ComputeNavmeshPin);

	if (!bComputeCollision &&
		!bComputeNavmesh)
	{
		return;
	}

	const FName InvokerChannel = GetConstantPin(Node.InvokerChannelPin);
	const int32 VoxelSize = GetConstantPin(Node.VoxelSizePin);
	const int32 ChunkSize = GetConstantPin(Node.ChunkSizePin);
	const int32 FullChunkSize = ChunkSize * VoxelSize;

	InvokerView = FVoxelInvokerManager::Get(GetWorld())->MakeView(
		InvokerChannel,
		FullChunkSize,
		0,
		GetLocalToWorld());

	InvokerView->Bind(
		MakeWeakPtrDelegate(this, [=](const TVoxelSet<FIntVector>& ChunksToAdd)
		{
			VOXEL_SCOPE_COUNTER("OnAddChunk");
			VOXEL_SCOPE_LOCK(CriticalSection);

			for (const FIntVector& ChunkKey : ChunksToAdd)
			{
				TSharedPtr<FChunk>& Chunk = Chunks_RequiresLock.FindOrAdd(ChunkKey);
				if (ensure(!Chunk))
				{
					Chunk = MakeVoxelShared<FChunk>();
				}

				const FVoxelBox Bounds = FVoxelBox(FVector(ChunkKey) * FullChunkSize, FVector(ChunkKey + 1) * FullChunkSize);

				TVoxelDynamicValueFactory<FVoxelMarchingCubeColliderWrapper> Factory(STATIC_FNAME("Output Marching Cube Collision & Navmesh"), [
					&Node = Node,
					VoxelSize,
					ChunkSize,
					Bounds](const FVoxelQuery& Query)
				{
					return Node.CreateCollider(Query, VoxelSize, ChunkSize, Bounds);
				});

				const TSharedRef<FVoxelQueryParameters> Parameters = MakeVoxelShared<FVoxelQueryParameters>();
				Parameters->Add<FVoxelLODQueryParameter>().LOD = 0;
				Chunk->Collider_RequiresLock = Factory
					.AddRef(NodeRef)
					.Priority(FVoxelTaskPriority::MakeBounds(
						Bounds,
						GetConstantPin(Node.PriorityOffsetPin),
						GetWorld(),
						GetLocalToWorld()))
					.Compute(GetTerminalGraphInstance(), Parameters);

				Chunk->Collider_RequiresLock.OnChanged(MakeWeakPtrLambda(this, [this, WeakChunk = MakeWeakPtr(Chunk)](const TSharedRef<const FVoxelMarchingCubeColliderWrapper>& Wrapper)
				{
					QueuedColliders.Enqueue({ WeakChunk, Wrapper->Collider });
				}));
			}
		}),
		MakeWeakPtrDelegate(this, [this](const TVoxelSet<FIntVector>& ChunksToRemove)
		{
			VOXEL_SCOPE_COUNTER("OnRemoveChunk");
			VOXEL_SCOPE_LOCK(CriticalSection);

			for (const FIntVector& ChunkKey : ChunksToRemove)
			{
				TSharedPtr<FChunk> Chunk;
				if (!ensure(Chunks_RequiresLock.RemoveAndCopyValue(ChunkKey, Chunk)))
				{
					return;
				}

				Chunk->Collider_RequiresLock = {};
				ChunksToDestroy.Enqueue(Chunk);
			}
		}));
}

void FVoxelExecNodeRuntime_OutputMarchingCubeCollisionAndNavmesh::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	InvokerView = {};

	ProcessChunksToDestroy();
	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
		for (const auto& It : Chunks_RequiresLock)
		{
			FChunk& Chunk = *It.Value;
			Chunk.Collider_RequiresLock = {};

			if (Runtime)
			{
				Runtime->DestroyComponent(Chunk.CollisionComponent_GameThread);
				Runtime->DestroyComponent(Chunk.NavmeshComponent_GameThread);
			}
			else
			{
				Chunk.CollisionComponent_GameThread.Reset();
				Chunk.NavmeshComponent_GameThread.Reset();
			}
		}
		Chunks_RequiresLock.Empty();
	}
	ProcessChunksToDestroy();
}

void FVoxelExecNodeRuntime_OutputMarchingCubeCollisionAndNavmesh::Tick(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!IsDestroyed());

	ProcessChunksToDestroy();
	ProcessQueuedColliders(Runtime);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_OutputMarchingCubeCollisionAndNavmesh::ProcessChunksToDestroy()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
	ensure(Runtime || IsDestroyed());

	TSharedPtr<FChunk> Chunk;
	while (ChunksToDestroy.Dequeue(Chunk))
	{
		if (!ensure(Chunk))
		{
			continue;
		}
		ensure(!Chunk->Collider_RequiresLock.IsValid());

		if (Runtime)
		{
			Runtime->DestroyComponent(Chunk->CollisionComponent_GameThread);
			Runtime->DestroyComponent(Chunk->NavmeshComponent_GameThread);
		}
		else
		{
			Chunk->CollisionComponent_GameThread.Reset();
			Chunk->NavmeshComponent_GameThread.Reset();
		}
	}
}

void FVoxelExecNodeRuntime_OutputMarchingCubeCollisionAndNavmesh::ProcessQueuedColliders(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensure(!IsDestroyed());

	FQueuedCollider QueuedCollider;
	while (QueuedColliders.Dequeue(QueuedCollider))
	{
		const TSharedPtr<FChunk> Chunk = QueuedCollider.Chunk.Pin();
		if (!Chunk)
		{
			continue;
		}

		const TSharedPtr<const FVoxelMarchingCubeCollider> Collider = QueuedCollider.Collider;
		if (!Collider)
		{
			Runtime.DestroyComponent(Chunk->CollisionComponent_GameThread);
			Runtime.DestroyComponent(Chunk->NavmeshComponent_GameThread);
			continue;
		}
		ensure(bComputeCollision || bComputeNavmesh);

		if (bComputeCollision)
		{
			UVoxelMarchingCubeCollisionComponent* Component = Chunk->CollisionComponent_GameThread.Get();
			if (!Component)
			{
				Component = Runtime.CreateComponent<UVoxelMarchingCubeCollisionComponent>();
				Chunk->CollisionComponent_GameThread = Component;
			}

			if (ensure(Component))
			{
				Component->SetRelativeLocation(Collider->Offset);
				Component->SetBodyInstance(*BodyInstance);
				Component->SetCollider(Collider);
			}
		}

		if (bComputeNavmesh)
		{
			UVoxelMarchingCubeNavmeshComponent* Component = Chunk->NavmeshComponent_GameThread.Get();
			if (!Component)
			{
				Component = Runtime.CreateComponent<UVoxelMarchingCubeNavmeshComponent>();
				Chunk->NavmeshComponent_GameThread = Component;
			}

			if (ensure(Component))
			{
				Component->SetRelativeLocation(Collider->Offset);
				Component->SetNavigationMesh(MakeVoxelShared<FVoxelMarchingCubeNavmesh>(
					Collider->LocalBounds,
					INLINE_LAMBDA
					{
						VOXEL_SCOPE_COUNTER("Build indices");

						TVoxelArray<int32> Indices;
						if (Collider->TriangleMesh->Elements().RequiresLargeIndices())
						{
							Indices = TVoxelArray<int32>(ReinterpretCastVoxelArrayView<int32>(Collider->TriangleMesh->Elements().GetLargeIndexBuffer()));
						}
						else
						{
						const TConstVoxelArrayView<uint16> ChaosIndices = ReinterpretCastVoxelArrayView<uint16>(Collider->TriangleMesh->Elements().GetSmallIndexBuffer());

						FVoxelUtilities::SetNumFast(Indices, ChaosIndices.Num());
						for (int32 Index = 0; Index < ChaosIndices.Num(); Index++)
						{
							Indices[Index] = ChaosIndices[Index];
						}
						}

						// Chaos triangles are reversed
						for (int32 Index = 0; Index < Indices.Num(); Index += 3)
						{
							Indices.Swap(Index + 0, Index + 2);
						}
								return Indices;
					},
					TVoxelArray<FVector3f>(ReinterpretCastVoxelArrayView<FVector3f>(Collider->TriangleMesh->Particles().XArray()))));
			}
		}
	}
}