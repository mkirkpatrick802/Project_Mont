// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelInvokerChunkSpawner.h"
#include "VoxelInvoker.h"
#include "VoxelRuntime.h"
#include "VoxelTaskGroup.h"
#include "VoxelTaskGroupScope.h"
#include "VoxelDependencyTracker.h"
#include "VoxelPositionQueryParameter.h"
#include "MarchingCube/VoxelExecNode_OutputMarchingCubeMesh.h"
#include "MarchingCube/VoxelExecNodeRuntime_OutputMarchingCubeMesh.h"

void FVoxelInvokerChunkSpawner::Initialize(const FVoxelExecNodeRuntime_OutputMarchingCubeMesh& NodeRuntime)
{
	ScaledChunkSize = GetVoxelSize() * ChunkSize;
	Referencer = MakeVoxelShared<FVoxelTaskReferencer>(STATIC_FNAME("InvokerChunkSpawner"));
	TerminalGraphInstance = NodeRuntime.GetTerminalGraphInstance();

	ComputeDistanceChecksSurface = NodeRuntime.GetNodeRuntime().GetCompute(
		NodeRuntime.Node.SurfacePin,
		NodeRuntime.GetTerminalGraphInstance());

	ComputeDistanceChecksTolerance = NodeRuntime.GetNodeRuntime().GetCompute(
		NodeRuntime.Node.DistanceChecksTolerancePin,
		NodeRuntime.GetTerminalGraphInstance());
}

void FVoxelInvokerChunkSpawner::Tick(const FVoxelExecNodeRuntime_OutputMarchingCubeMesh& NodeRuntime)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (InvokerView_GameThread &&
		InvokerView_GameThread->ChunkSize == ScaledChunkSize * GroupSize)
	{
		return;
	}

	VOXEL_SCOPE_LOCK(CriticalSection);

	BulkChunkRefs_RequiresLock.Empty();

	InvokerView_GameThread = FVoxelInvokerManager::Get(NodeRuntime.GetWorld())->MakeView(
		InvokerChannel,
		ScaledChunkSize * GroupSize,
		0,
		NodeRuntime.GetLocalToWorld());

	InvokerViewBindRef_GameThread = MakeSharedVoid();

	InvokerView_GameThread->Bind_Async(
		MakeWeakPtrDelegate(InvokerViewBindRef_GameThread, MakeWeakPtrLambda(this, [=](const TVoxelSet<FIntVector>& ChunksToAdd)
		{
			VOXEL_SCOPE_COUNTER("OnAddChunk");
			VOXEL_SCOPE_LOCK(CriticalSection);

			const FVoxelBox WorldBounds(-WorldSize, WorldSize);
			for (const FIntVector& Chunk : ChunksToAdd)
			{
				if (!FVoxelIntBox(Chunk).Scale(ScaledChunkSize * GroupSize).ToVoxelBox().Intersect(WorldBounds))
				{
					continue;
				}

				const TSharedRef<FBulkChunk> BulkChunk = MakeVoxelShared<FBulkChunk>();

				BulkChunkRefs_RequiresLock.Add_EnsureNew(Chunk, BulkChunk);

				UpdateBulkChunk(Chunk, BulkChunk);
			}
		})),
		MakeWeakPtrDelegate(InvokerViewBindRef_GameThread, MakeWeakPtrLambda(this, [this](const TVoxelSet<FIntVector>& ChunksToRemove)
		{
			VOXEL_SCOPE_COUNTER("OnRemoveChunk");
			VOXEL_SCOPE_LOCK(CriticalSection);

			const FVoxelBox WorldBounds(-WorldSize, WorldSize);
			for (const FIntVector& Chunk : ChunksToRemove)
			{
				if (!FVoxelIntBox(Chunk).Scale(ScaledChunkSize * GroupSize).ToVoxelBox().Intersect(WorldBounds))
				{
					continue;
				}

				ensure(BulkChunkRefs_RequiresLock.Remove(Chunk));
			}
		})));
}

void FVoxelInvokerChunkSpawner::UpdateBulkChunk(
	const FIntVector& Chunk,
	const TSharedRef<FBulkChunk>& InBulkChunk) const
{
	VOXEL_FUNCTION_COUNTER();

	TSharedPtr<FVoxelTaskGroup> TaskGroup;
	{
		VOXEL_SCOPE_LOCK(InBulkChunk->CriticalSection);

		ensure(!InBulkChunk->TaskGroup_RequiresLock);
		InBulkChunk->TaskGroup_RequiresLock = FVoxelTaskGroup::Create(
			STATIC_FNAME("InvokerChunkSpawner"),
			FVoxelTaskPriority::MakeTop(),
			Referencer.ToSharedRef(),
			TerminalGraphInstance.ToSharedRef());

		TaskGroup = InBulkChunk->TaskGroup_RequiresLock;
	}

	FVoxelTaskGroupScope Scope;
	if (!ensure(Scope.Initialize(*TaskGroup)))
	{
		return;
	}

	MakeVoxelTask("StartDistanceChecks")
	.Execute(MakeWeakPtrLambda(this, [this, Chunk, WeakBulkChunk = MakeWeakPtr(InBulkChunk)]
	{
		const float Step = ScaledChunkSize / 2.f;

		const TSharedRef<FVoxelQueryParameters> Parameters = MakeVoxelShared<FVoxelQueryParameters>();
		Parameters->Add<FVoxelLODQueryParameter>().LOD = 0;
		Parameters->Add<FVoxelSurfaceQueryParameter>().ComputeDistance();
		Parameters->Add<FVoxelMinExactDistanceQueryParameter>().MinExactDistance = ScaledChunkSize;
		Parameters->Add<FVoxelPositionQueryParameter>().InitializeGrid(
			FVector3f(Chunk * GroupSize * ScaledChunkSize) + Step / 2.f,
			Step,
			FIntVector(GroupSize * 2));

		const TSharedRef<FVoxelDependencyTracker> DependencyTracker = FVoxelDependencyTracker::Create(STATIC_FNAME("InvokerChunkSpawner"));
		const FVoxelQuery Query = FVoxelQuery::Make(
			TerminalGraphInstance.ToSharedRef(),
			Parameters,
			DependencyTracker);

		TVoxelFutureValue<FVoxelSurface> FutureSurface = (*ComputeDistanceChecksSurface)(Query);
		if (!ensure(FutureSurface.IsValid()))
		{
			FutureSurface = FVoxelSurface();
		}
		const TVoxelFutureValue<float> FutureTolerance = (*ComputeDistanceChecksTolerance)(Query);

		MakeVoxelTask("ProcessDistanceChecks")
		.Dependencies(FutureSurface, FutureTolerance)
		.Execute(MakeWeakPtrLambda(this, [=]
		{
			FVoxelFloatBuffer Distances = FutureSurface.Get_CheckCompleted().GetDistance();
			if (!ensure(Distances.IsConstant() || Distances.Num() == FMath::Cube(2 * GroupSize)))
			{
				Distances = 1.e6f;
			}

			const TSharedPtr<FBulkChunk> BulkChunk = WeakBulkChunk.Pin();
			if (!BulkChunk)
			{
				return;
			}

			{
				VOXEL_SCOPE_LOCK(BulkChunk->CriticalSection);

				const float Threshold = ScaledChunkSize / 4.f * UE_SQRT_2 * (1.f + FutureTolerance.Get_CheckCompleted());

				for (int32 Z = 0; Z < GroupSize; Z++)
				{
					for (int32 Y = 0; Y < GroupSize; Y++)
					{
						for (int32 X = 0; X < GroupSize; X++)
						{
							bool bCanSkip = true;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 0, 2 * Y + 0, 2 * Z + 0)]) > Threshold;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 1, 2 * Y + 0, 2 * Z + 0)]) > Threshold;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 0, 2 * Y + 1, 2 * Z + 0)]) > Threshold;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 1, 2 * Y + 1, 2 * Z + 0)]) > Threshold;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 0, 2 * Y + 0, 2 * Z + 1)]) > Threshold;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 1, 2 * Y + 0, 2 * Z + 1)]) > Threshold;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 0, 2 * Y + 1, 2 * Z + 1)]) > Threshold;
							bCanSkip &= FMath::Abs(Distances[FVoxelUtilities::Get3DIndex<int32>(2 * GroupSize, 2 * X + 1, 2 * Y + 1, 2 * Z + 1)]) > Threshold;

							if (bCanSkip)
							{
								BulkChunk->ChunkRefs_RequiresLock.Remove(FIntVector(X, Y, Z));
								continue;
							}

							const FVoxelBox WorldBounds(-WorldSize, WorldSize);
							const FVoxelBox Bounds =
								FVoxelIntBox(X, Y, Z)
								.ShiftBy(Chunk * GroupSize)
								.ToVoxelBox()
								.Scale(ScaledChunkSize);

							if (!WorldBounds.Intersect(Bounds))
							{
								continue;
							}

							TSharedPtr<FVoxelChunkRef>& ChunkRef = BulkChunk->ChunkRefs_RequiresLock.FindOrAdd(FIntVector(X, Y, Z));
							if (ChunkRef)
							{
								continue;
							}

							ChunkRef = CreateChunk(0, ChunkSize, Bounds);
							ChunkRef->Compute();
						}
					}
				}

				BulkChunk->TaskGroup_RequiresLock.Reset();

				ensure(!BulkChunk->DependencyTracker_RequiresLock);
				BulkChunk->DependencyTracker_RequiresLock = DependencyTracker;
			}

			DependencyTracker->SetOnInvalidated(MakeWeakPtrLambda(this, [this, Chunk, WeakBulkChunk]
			{
				const TSharedPtr<FBulkChunk> PinnedBulkChunk = WeakBulkChunk.Pin();
				if (!PinnedBulkChunk)
				{
					return;
				}

				{
					VOXEL_SCOPE_LOCK(PinnedBulkChunk->CriticalSection);
					PinnedBulkChunk->TaskGroup_RequiresLock.Reset();
					PinnedBulkChunk->DependencyTracker_RequiresLock.Reset();
				}

				UpdateBulkChunk(Chunk, PinnedBulkChunk.ToSharedRef());
			}));
		}));
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_MakeInvokerChunkSpawner, Spawner)
{
	const TValue<int32> LOD = Get(LODPin, Query);
	const TValue<float> WorldSize = Get(WorldSizePin, Query);
	const TValue<FName> InvokerChannel = Get(InvokerChannelPin, Query);
	const TValue<int32> ChunkSize = Get(ChunkSizePin, Query);
	const TValue<int32> GroupSize = Get(GroupSizePin, Query);

	return VOXEL_ON_COMPLETE(LOD, WorldSize, InvokerChannel, ChunkSize, GroupSize)
	{
		const TSharedRef<FVoxelInvokerChunkSpawner> Spawner = MakeVoxelShared<FVoxelInvokerChunkSpawner>();
		Spawner->NodeRef = GetNodeRef();
		Spawner->LOD = FMath::Clamp(LOD, 0, 30);
		Spawner->WorldSize = WorldSize;
		Spawner->InvokerChannel = InvokerChannel;
		Spawner->ChunkSize = FMath::Clamp(FMath::CeilToInt(ChunkSize / 2.f) * 2, 4, 128);
		Spawner->GroupSize = FMath::Clamp(GroupSize, 1, 128);
		return Spawner;
	};
}