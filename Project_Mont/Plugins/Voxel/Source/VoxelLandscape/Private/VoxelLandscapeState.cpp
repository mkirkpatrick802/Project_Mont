// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscapeState.h"
#include "VoxelLandscape.h"
#include "VoxelLandscapeChunkTree.h"
#include "VoxelLandscapeBrushManager.h"
#include "Nanite/VoxelLandscapeNaniteMesh.h"
#include "Nanite/VoxelLandscapeNaniteMesher.h"
#include "Height/VoxelLandscapeHeightMesh.h"
#include "Height/VoxelLandscapeHeightMesher.h"
#include "Volume/VoxelLandscapeVolumeMesh.h"
#include "Volume/VoxelLandscapeVolumeMesher.h"

FVoxelLandscapeState::FVoxelLandscapeState(
	const AVoxelLandscape& Actor,
	const FVector& CameraPosition,
	const TSharedPtr<const FVoxelLandscapeState>& LastState,
	const TOptional<FVoxelBox2D>& InvalidatedHeightBounds,
	const TOptional<FVoxelBox>& InvalidatedVolumeBounds)
	: World(Actor.GetWorld())
	, CameraPosition(CameraPosition)

	, VoxelToWorld(FScaleMatrix(Actor.VoxelSize) * Actor.ActorToWorld().ToMatrixWithScale())
	, WorldToVoxel(VoxelToWorld.Inverse())
	, VoxelToWorld2D(FVoxelUtilities::MakeTransform2(VoxelToWorld))
	, WorldToVoxel2D(FVoxelUtilities::MakeTransform2(WorldToVoxel))

	, VoxelSize(Actor.VoxelSize)
	, ChunkSize(32)
	, TargetVoxelSizeOnScreen(Actor.TargetVoxelSizeOnScreen)
	, DetailTextureSize(FMath::Clamp(Actor.DetailTextureSize, 2, 16))
	, bEnableNanite(Actor.bEnableNanite)

	, Material(FVoxelMaterialRef::Make(Actor.Material))

	, LastState(LastState)
	, InvalidatedHeightBounds(InvalidatedHeightBounds)
	, InvalidatedVolumeBounds(InvalidatedVolumeBounds)
{
	// Landscape shouldn't be scaled
	ensure(Actor.ActorToWorld().GetScale3D().Equals(FVector::OneVector));
}

void FVoxelLandscapeState::Compute()
{
	VOXEL_FUNCTION_COUNTER();

	const TVoxelFuture<FVoxelLandscapeBrushes> FutureBrushes = FVoxelLandscapeBrushManager::Get(World)->GetBrushes(WorldToVoxel);

	FutureBrushes.Then_VoxelThread(MakeWeakPtrLambda(this, [=](const TSharedRef<FVoxelLandscapeBrushes>& InBrushes)
	{
		VOXEL_FUNCTION_COUNTER();

		Brushes = InBrushes;

		if (Brushes->IsEmpty())
		{
			bIsReadyToRender.Set(true);
			return;
		}

		const TSharedRef<FVoxelLandscapeChunkTree> Tree = MakeVoxelShared<FVoxelLandscapeChunkTree>(*this);
		Tree->Build();

		ChunkKeyToHeightMesh.Reserve(Tree->ChunkKeys.Num());

		TVoxelArray<TVoxelFuture<FVoxelLandscapeNaniteMesh>> FutureNaniteMeshes;
		TVoxelArray<TVoxelFuture<FVoxelLandscapeHeightMesh>> FutureHeightMeshes;
		TVoxelArray<TVoxelFuture<FVoxelLandscapeVolumeMeshData>> FutureVolumeMeshDatas;
		for (const FVoxelLandscapeChunkKeyWithTransition& ChunkKeyWithTransition : Tree->ChunkKeys)
		{
			const FVoxelLandscapeChunkKey ChunkKey = ChunkKeyWithTransition.ChunkKey;

			const bool bRestored = INLINE_LAMBDA
			{
				if (!LastState)
				{
					return false;
				}

				if (InvalidatedHeightBounds &&
					InvalidatedHeightBounds->Intersect(FVoxelBox2D(ChunkKey.GetQueriedBounds2D(*this))))
				{
					return false;
				}

				if (InvalidatedVolumeBounds &&
					InvalidatedVolumeBounds->Intersect(ChunkKey.GetQueriedBounds(*this)))
				{
					return false;
				}

				if (LastState->EmptyChunks.Contains(ChunkKey))
				{
					EmptyChunks.Add_EnsureNew(ChunkKey);
					return true;
				}

				if (const TSharedPtr<FVoxelLandscapeNaniteMesh> LastMesh = LastState->ChunkKeyToNaniteMesh.FindRef(ChunkKey))
				{
					ChunkKeyToNaniteMesh.Add_EnsureNew(ChunkKey, LastMesh.ToSharedRef());
					return true;
				}

				if (const TSharedPtr<FVoxelLandscapeHeightMesh> LastMesh = LastState->ChunkKeyToHeightMesh.FindRef(ChunkKey))
				{
					ChunkKeyToHeightMesh.Add_EnsureNew(ChunkKey, LastMesh.ToSharedRef());
					return true;
				}

				if (const TSharedPtr<FVoxelLandscapeVolumeMesh> LastMesh = LastState->ChunkKeyToVolumeMesh.FindRef(ChunkKey))
				{
					ChunkKeyToVolumeMesh.Add_EnsureNew(ChunkKey, LastMesh.ToSharedRef());
					return true;
				}

				return false;
			};

			if (bRestored)
			{
				continue;
			}

			if (bEnableNanite)
			{
				const TSharedRef<FVoxelLandscapeNaniteMesher> Mesher = MakeVoxelShared<FVoxelLandscapeNaniteMesher>(ChunkKey, AsShared());
				FutureNaniteMeshes.Add(AsyncVoxelTask([=]
				{
					return Mesher->Build();
				}));

				continue;
			}

			if (!Brushes->HasVolumeBrushes(ChunkKey.GetQueriedBounds(*this)))
			{
				const TSharedRef<FVoxelLandscapeHeightMesher> Mesher = MakeVoxelShared<FVoxelLandscapeHeightMesher>(ChunkKey, AsShared());
				FutureHeightMeshes.Add(AsyncVoxelTask([=]
				{
					return Mesher->Build();
				}));

				continue;
			}

			const TSharedRef<FVoxelLandscapeVolumeMesher> Mesher = MakeVoxelShared<FVoxelLandscapeVolumeMesher>(ChunkKey, AsShared());
			FutureVolumeMeshDatas.Add(AsyncVoxelTask([=]
			{
				return Mesher->Build();
			}));
		}

		TVoxelArray<FVoxelFuture> AllFutures;
		AllFutures.Append(FutureNaniteMeshes);
		AllFutures.Append(FutureHeightMeshes);
		AllFutures.Append(FutureVolumeMeshDatas);

		FVoxelFuture(AllFutures).Then_AnyThread(MakeStrongPtrLambda(this, [=]
		{
			TVoxelArray<TSharedPtr<FVoxelLandscapeHeightMesh>> HeightMeshesToInitialize;
			TVoxelArray<TSharedPtr<FVoxelLandscapeVolumeMesh>> VolumeMeshesToInitialize;
			HeightMeshesToInitialize.Reserve(FutureHeightMeshes.Num());
			VolumeMeshesToInitialize.Reserve(FutureVolumeMeshDatas.Num());

			for (const TVoxelFuture<FVoxelLandscapeNaniteMesh>& FutureMesh : FutureNaniteMeshes)
			{
				const TSharedRef<FVoxelLandscapeNaniteMesh> Mesh = FutureMesh.GetValueChecked();
				if (Mesh->bIsEmpty)
				{
					EmptyChunks.Add_EnsureNew(Mesh->ChunkKey);
					continue;
				}

				ChunkKeyToNaniteMesh.Add_EnsureNew(Mesh->ChunkKey, Mesh);
			}

			for (const TVoxelFuture<FVoxelLandscapeHeightMesh>& FutureMesh : FutureHeightMeshes)
			{
				const TSharedRef<FVoxelLandscapeHeightMesh> Mesh = FutureMesh.GetValueChecked();
				if (!Mesh->ShouldRender())
				{
					EmptyChunks.Add_EnsureNew(Mesh->ChunkKey);
					continue;
				}

				HeightMeshesToInitialize.Add(Mesh);
				ChunkKeyToHeightMesh.Add_EnsureNew(Mesh->ChunkKey, Mesh);
			}

			for (const TVoxelFuture<FVoxelLandscapeVolumeMeshData>& FutureMeshData : FutureVolumeMeshDatas)
			{
				const FVoxelLandscapeVolumeMeshData& MeshData = *FutureMeshData.GetValueChecked();
				if (MeshData.Cells.Num() == 0)
				{
					EmptyChunks.Add_EnsureNew(MeshData.ChunkKey);
					continue;
				}

				const TSharedRef<FVoxelLandscapeVolumeMesh> Mesh = MeshData.CreateMesh(0);

				VolumeMeshesToInitialize.Add(Mesh);
				ChunkKeyToVolumeMesh.Add_EnsureNew(Mesh->ChunkKey, Mesh);
			}

			VOXEL_ENQUEUE_RENDER_COMMAND(InitializeLandscapeMeshes)(MakeStrongPtrLambda(this, [=](FRHICommandListBase& RHICmdList)
			{
				for (const TSharedPtr<FVoxelLandscapeHeightMesh>& Mesh : HeightMeshesToInitialize)
				{
					Mesh->Initialize_RenderThread(RHICmdList, *this);
				}
				for (const TSharedPtr<FVoxelLandscapeVolumeMesh>& Mesh : VolumeMeshesToInitialize)
				{
					Mesh->Initialize_RenderThread(RHICmdList, *this);
				}

				// Avoid keeping every state alive
				LastState.Reset();

				bIsReadyToRender.Set(true);
			}));
		}));
	}));
}