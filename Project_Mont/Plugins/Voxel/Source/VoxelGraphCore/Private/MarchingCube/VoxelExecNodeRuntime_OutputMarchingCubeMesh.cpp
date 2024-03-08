// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelExecNodeRuntime_OutputMarchingCubeMesh.h"
#include "MarchingCube/VoxelMarchingCubeMesh.h"
#include "MarchingCube/VoxelMarchingCubeCollider.h"
#include "MarchingCube/VoxelMarchingCubeMeshComponent.h"
#include "MarchingCube/VoxelMarchingCubeCollisionComponent.h"
#include "VoxelRuntime.h"
#include "VoxelScreenSizeChunkSpawner.h"

void FVoxelExecNodeRuntime_OutputMarchingCubeMesh::Create()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
	if (!ensure(Runtime))
	{
		return;
	}

	ChunkSpawner = GetConstantPin(Node.ChunkSpawnerPin)->MakeSharedCopy();
	VoxelSize = GetConstantPin(Node.VoxelSizePin);

	if (ChunkSpawner->GetStruct() == StaticStructFast<FVoxelChunkSpawner>())
	{
		const TSharedRef<FVoxelScreenSizeChunkSpawner> ScreenSizeChunkSpawner = MakeVoxelShared<FVoxelScreenSizeChunkSpawner>();
		ScreenSizeChunkSpawner->LastChunkScreenSize = 1.f;
		ChunkSpawner = ScreenSizeChunkSpawner;
	}

	ChunkSpawner->PrivateRuntimeInfo = GetRuntimeInfo();
	ChunkSpawner->PrivateVoxelSize = VoxelSize;
	ChunkSpawner->PrivateCreateChunkLambda = MakeWeakPtrLambda(this, [this](
		const int32 LOD,
		const int32 ChunkSize,
		const FVoxelBox& Bounds) -> TSharedPtr<FVoxelChunkRef>
	{
		const TSharedRef<FChunkInfo> ChunkInfo = MakeVoxelShared<FChunkInfo>(LOD, ChunkSize, Bounds);

		VOXEL_SCOPE_LOCK(ChunkInfos_CriticalSection);
		ChunkInfos.Add(ChunkInfo->ChunkId, ChunkInfo);

		return MakeVoxelShared<FVoxelChunkRef>(ChunkInfo->ChunkId, ChunkActionQueue);
	});

	ChunkSpawner->Initialize(*this);
}

void FVoxelExecNodeRuntime_OutputMarchingCubeMesh::Destroy()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(ChunkInfos_CriticalSection);

	const TSharedPtr<FVoxelRuntime> Runtime = GetRuntime();
	if (!Runtime)
	{
		// Already destroyed
		for (const auto& It : ChunkInfos)
		{
			It.Value->Mesh = {};
			It.Value->MeshComponent = {};
			It.Value->CollisionComponent = {};
			It.Value->FlushOnComplete();
		}
		ChunkInfos.Empty();
		return;
	}

	TArray<FVoxelChunkId> ChunkIds;
	ChunkInfos.GenerateKeyArray(ChunkIds);

	for (const FVoxelChunkId ChunkId : ChunkIds)
	{
		ProcessAction(&*Runtime, FVoxelChunkAction(EVoxelChunkAction::Destroy, ChunkId));
	}

	ensure(ChunkInfos.Num() == 0);
}

void FVoxelExecNodeRuntime_OutputMarchingCubeMesh::Tick(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(ChunkSpawner))
	{
		return;
	}

	ChunkSpawner->Tick(*this);

	ProcessMeshes(Runtime);
	ProcessActions(&Runtime, true);

	if (ProcessActionsGraphEvent.IsValid())
	{
		VOXEL_SCOPE_COUNTER("Wait");
		ProcessActionsGraphEvent->Wait();
	}
	ProcessActionsGraphEvent = TGraphTask<TVoxelGraphTask<ENamedThreads::AnyBackgroundThreadNormalTask>>::CreateTask().ConstructAndDispatchWhenReady(MakeWeakPtrLambda(this, [this]
	{
		ProcessActions(nullptr, false);
	}));
}

void FVoxelExecNodeRuntime_OutputMarchingCubeMesh::FChunkInfo::FlushOnComplete()
{
	if (OnCompleteArray.Num() == 0)
	{
		return;
	}

	AsyncBackgroundTask([OnCompleteArray = MoveTemp(OnCompleteArray)]
	{
		for (const TSharedPtr<const TVoxelUniqueFunction<void()>>& OnComplete : OnCompleteArray)
		{
			(*OnComplete)();
		}
	});
}

void FVoxelExecNodeRuntime_OutputMarchingCubeMesh::ProcessMeshes(FVoxelRuntime& Runtime)
{
	VOXEL_FUNCTION_COUNTER();

	TArray<TSharedPtr<const TVoxelUniqueFunction<void()>>> OnCompleteArray;
	ON_SCOPE_EXIT
	{
		if (OnCompleteArray.Num() > 0)
		{
			AsyncBackgroundTask([OnCompleteArray = MoveTemp(OnCompleteArray)]
			{
				for (const TSharedPtr<const TVoxelUniqueFunction<void()>>& OnComplete : OnCompleteArray)
				{
					(*OnComplete)();
				}
			});
		}
	};

	VOXEL_SCOPE_LOCK(ChunkInfos_CriticalSection);

	FQueuedMesh QueuedMesh;
	while (QueuedMeshes->Dequeue(QueuedMesh))
	{
		const TSharedPtr<FChunkInfo> ChunkInfo = ChunkInfos.FindRef(QueuedMesh.ChunkId);
		if (!ChunkInfo)
		{
			continue;
		}

		const TSharedPtr<FVoxelMarchingCubeMesh> Mesh = QueuedMesh.Mesh->Mesh;
		const TSharedPtr<const FVoxelMarchingCubeCollider> Collider = QueuedMesh.Mesh->Collider;

		if (!Mesh)
		{
			Runtime.DestroyComponent(ChunkInfo->MeshComponent);
		}
		else
		{
			VOXEL_ENQUEUE_RENDER_COMMAND(SetTransitionMask_RenderThread)([Mesh, TransitionMask = ChunkInfo->TransitionMask](FRHICommandList& RHICmdList)
			{
				Mesh->SetTransitionMask_RenderThread(RHICmdList, TransitionMask);
			});

			if (!ChunkInfo->MeshComponent.IsValid())
			{
				ChunkInfo->MeshComponent = Runtime.CreateComponent<UVoxelMarchingCubeMeshComponent>();
			}

			UVoxelMarchingCubeMeshComponent* Component = ChunkInfo->MeshComponent.Get();
			if (ensure(Component))
			{
				Component->SetRelativeLocation(ChunkInfo->Bounds.Min);
				Component->SetMesh(Mesh);
				QueuedMesh.Mesh->MeshSettings->ApplyToComponent(*Component);
			}
		}

		if (!Collider)
		{
			Runtime.DestroyComponent(ChunkInfo->CollisionComponent);
		}
		else
		{
			if (!ChunkInfo->CollisionComponent.IsValid())
			{
				ChunkInfo->CollisionComponent = Runtime.CreateComponent<UVoxelMarchingCubeCollisionComponent>();
			}

			UVoxelMarchingCubeCollisionComponent* Component = ChunkInfo->CollisionComponent.Get();
			if (ensure(Component))
			{
				Component->SetRelativeLocation(Collider->Offset);
				Component->SetBodyInstance(*QueuedMesh.Mesh->BodyInstance);
				if (!IsGameWorld())
				{
					Component->BodyInstance.SetResponseToChannel(ECC_EngineTraceChannel6, ECR_Block);
				}
				Component->SetCollider(Collider);
			}
		}

		OnCompleteArray.Append(ChunkInfo->OnCompleteArray);
		ChunkInfo->OnCompleteArray.Empty();
	}
}

void FVoxelExecNodeRuntime_OutputMarchingCubeMesh::ProcessActions(FVoxelRuntime* Runtime, const bool bIsInGameThread)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(ChunkInfos_CriticalSection);
	ensure(bIsInGameThread == IsInGameThread());
	ensure(bIsInGameThread == (Runtime != nullptr));

	const double StartTime = FPlatformTime::Seconds();

	FVoxelChunkAction Action;
	while ((bIsInGameThread ? ChunkActionQueue->GameQueue : ChunkActionQueue->AsyncQueue).Dequeue(Action))
	{
		ProcessAction(Runtime, Action);

		if (FPlatformTime::Seconds() - StartTime > 0.1f)
		{
			break;
		}
	}
}

void FVoxelExecNodeRuntime_OutputMarchingCubeMesh::ProcessAction(FVoxelRuntime* Runtime, const FVoxelChunkAction& Action)
{
	checkVoxelSlow(ChunkInfos_CriticalSection.IsLocked());

	const TSharedPtr<FChunkInfo> ChunkInfo = ChunkInfos.FindRef(Action.ChunkId);
	if (!ChunkInfo)
	{
		return;
	}

	switch (Action.Action)
	{
	default: ensure(false);
	case EVoxelChunkAction::Compute:
	{
		VOXEL_SCOPE_COUNTER("Compute");

		TVoxelDynamicValueFactory<FVoxelMarchingCubeExecNodeMesh> Factory(STATIC_FNAME("Output Marching Cube Mesh"), [
			&Node = Node,
			VoxelSize = VoxelSize,
			ChunkSize = ChunkInfo->ChunkSize,
			Bounds = ChunkInfo->Bounds](const FVoxelQuery& Query)
		{
			static FVoxelCounter32 ChunkId = 0;
			const FName Name(FString::Printf(TEXT("ChunkId = %d"), ChunkId.Increment_ReturnNew()));
			const TVoxelUniquePtr<FVoxelTaskTagScope> Scope = FVoxelTaskTagScope::Create(Name);

			checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(&Node));
			return Node.CreateMesh(Query, VoxelSize, ChunkSize, Bounds);
		});

		const TSharedRef<FVoxelQueryParameters> Parameters = MakeVoxelShared<FVoxelQueryParameters>();
		Parameters->Add<FVoxelLODQueryParameter>().LOD = ChunkInfo->LOD;

		ensure(!ChunkInfo->Mesh.IsValid());
		ChunkInfo->Mesh = Factory
			.AddRef(NodeRef)
			.Priority(FVoxelTaskPriority::MakeBounds(
				ChunkInfo->Bounds,
				GetConstantPin(Node.PriorityOffsetPin),
				GetWorld(),
				GetLocalToWorld()))
			.Compute(GetTerminalGraphInstance(), Parameters);

		ChunkInfo->Mesh.OnChanged([QueuedMeshes = QueuedMeshes, ChunkId = ChunkInfo->ChunkId](const TSharedRef<const FVoxelMarchingCubeExecNodeMesh>& NewMesh)
		{
			QueuedMeshes->Enqueue(FQueuedMesh{ ChunkId, NewMesh });
		});

		if (Action.OnComputeComplete)
		{
			ChunkInfo->OnCompleteArray.Add(Action.OnComputeComplete);
		}
	}
	break;
	case EVoxelChunkAction::SetTransitionMask:
	{
		VOXEL_SCOPE_COUNTER("SetTransitionMask");
		check(IsInGameThread());

		ChunkInfo->TransitionMask = Action.TransitionMask;

		if (UVoxelMarchingCubeMeshComponent* Component = ChunkInfo->MeshComponent.Get())
		{
			if (const TSharedPtr<FVoxelMarchingCubeMesh> Mesh = Component->GetMesh())
			{
				Mesh->SetTransitionMask_GameThread(Action.TransitionMask);
			}
		}
	}
	break;
	case EVoxelChunkAction::BeginDestroy:
	{
		VOXEL_SCOPE_COUNTER("BeginDestroy");

		ChunkInfo->Mesh = {};
	}
	break;
	case EVoxelChunkAction::Destroy:
	{
		VOXEL_SCOPE_COUNTER("Destroy");
		check(IsInGameThread());
		check(Runtime);

		ChunkInfo->Mesh = {};

		Runtime->DestroyComponent(ChunkInfo->MeshComponent);
		Runtime->DestroyComponent(ChunkInfo->CollisionComponent);

		ChunkInfo->FlushOnComplete();

		ChunkInfos.Remove(Action.ChunkId);

		ensure(ChunkInfo.GetSharedReferenceCount() == 1);
	}
	break;
	}
}