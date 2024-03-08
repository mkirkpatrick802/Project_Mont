// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelMarchingCubeCollisionSceneProxy.h"
#include "MarchingCube/VoxelMarchingCubeCollider.h"
#include "MarchingCube/VoxelMarchingCubeCollisionComponent.h"
#include "MeshBatch.h"
#include "SceneManagement.h"
#include "LocalVertexFactory.h"
#include "Chaos/TriangleMeshImplicitObject.h"

FVoxelMarchingCubeColliderRenderData::FVoxelMarchingCubeColliderRenderData(const FVoxelMarchingCubeCollider& Collider)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<uint32> Indices;
	TVoxelArray<FVector3f> Vertices;
	{
		const auto Process = [&](auto Elements)
		{
			for (const auto& Element : Elements)
			{
				Indices.Add(Vertices.Num());
				Vertices.Add(Collider.TriangleMesh->Particles().X(Element[0]));

				Indices.Add(Vertices.Num());
				Vertices.Add(Collider.TriangleMesh->Particles().X(Element[1]));

				Indices.Add(Vertices.Num());
				Vertices.Add(Collider.TriangleMesh->Particles().X(Element[2]));
			}
		};

		if (Collider.TriangleMesh->Elements().RequiresLargeIndices())
		{
			Process(Collider.TriangleMesh->Elements().GetLargeIndexBuffer());
		}
		else
		{
			Process(Collider.TriangleMesh->Elements().GetSmallIndexBuffer());
		}
	}

	IndexBuffer.SetIndices(Indices.ToConstArray(), EIndexBufferStride::Force32Bit);
	PositionVertexBuffer.Init(Vertices.Num(), false);
	StaticMeshVertexBuffer.Init(Vertices.Num(), 1, false);
	ColorVertexBuffer.Init(Vertices.Num(), false);

	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		PositionVertexBuffer.VertexPosition(Index) = Vertices[Index];
		StaticMeshVertexBuffer.SetVertexUV(Index, 0, FVector2f::ZeroVector);
		ColorVertexBuffer.VertexColor(Index) = FColor::Black;
	}

	for (int32 Index = 0; Index < Vertices.Num(); Index += 3)
	{
		const FVector3f Normal = FVoxelUtilities::GetTriangleNormal(
			Vertices[Index + 0],
			Vertices[Index + 1],
			Vertices[Index + 2]);

		const FVector3f Tangent = FVector3f::ForwardVector;
		const FVector3f Bitangent = FVector3f::CrossProduct(Normal, Tangent);

		StaticMeshVertexBuffer.SetVertexTangents(Index + 0, Tangent, Bitangent, Normal);
		StaticMeshVertexBuffer.SetVertexTangents(Index + 1, Tangent, Bitangent, Normal);
		StaticMeshVertexBuffer.SetVertexTangents(Index + 2, Tangent, Bitangent, Normal);
	}

	FRHICommandListImmediate& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	IndexBuffer.InitResource(UE_503_ONLY(RHICmdList));
	PositionVertexBuffer.InitResource(UE_503_ONLY(RHICmdList));
	StaticMeshVertexBuffer.InitResource(UE_503_ONLY(RHICmdList));
	ColorVertexBuffer.InitResource(UE_503_ONLY(RHICmdList));

	VertexFactory = MakeUnique<FLocalVertexFactory>(GMaxRHIFeatureLevel, "FVoxelMarchingCubeCollider_Chaos_RenderData");

	FLocalVertexFactory::FDataType Data;
	PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory.Get(), Data);
	StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactory.Get(), Data);
	ColorVertexBuffer.BindColorVertexBuffer(VertexFactory.Get(), Data);

	VertexFactory->SetData(UE_504_ONLY(RHICmdList,) Data);
	VertexFactory->InitResource(UE_503_ONLY(RHICmdList));
}

FVoxelMarchingCubeColliderRenderData::~FVoxelMarchingCubeColliderRenderData()
{
	IndexBuffer.ReleaseResource();
	PositionVertexBuffer.ReleaseResource();
	StaticMeshVertexBuffer.ReleaseResource();
	ColorVertexBuffer.ReleaseResource();
	VertexFactory->ReleaseResource();
}

void FVoxelMarchingCubeColliderRenderData::Draw_RenderThread(FMeshBatch& MeshBatch) const
{
	MeshBatch.Type = PT_TriangleList;
	MeshBatch.VertexFactory = VertexFactory.Get();

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = &IndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = IndexBuffer.GetNumIndices() / 3;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = PositionVertexBuffer.GetNumVertices() - 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMarchingCubeCollisionSceneProxy::FVoxelMarchingCubeCollisionSceneProxy(const UVoxelMarchingCubeCollisionComponent& Component)
	: FPrimitiveSceneProxy(&Component)
	, Collider(Component.Collider.ToSharedRef())
{
}

void FVoxelMarchingCubeCollisionSceneProxy::DestroyRenderThreadResources()
{
	VOXEL_FUNCTION_COUNTER();
	RenderData.Reset();
}

void FVoxelMarchingCubeCollisionSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, const uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	VOXEL_FUNCTION_COUNTER();

	if (!RenderData)
	{
		RenderData = MakeVoxelShared<FVoxelMarchingCubeColliderRenderData>(*Collider);
	}

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		if (!(VisibilityMap & (1 << ViewIndex)))
		{
			continue;
		}

		ensure(
			Views[ViewIndex]->Family->EngineShowFlags.Collision ||
			Views[ViewIndex]->Family->EngineShowFlags.CollisionPawn ||
			Views[ViewIndex]->Family->EngineShowFlags.CollisionVisibility);

		FMeshBatch& MeshBatch = Collector.AllocateMesh();
		MeshBatch.MaterialRenderProxy = FVoxelRenderUtilities::CreateColoredRenderProxy(Collector, GetWireframeColor());
		MeshBatch.ReverseCulling = IsLocalToWorldDeterminantNegative();
		MeshBatch.bDisableBackfaceCulling = true;
		MeshBatch.DepthPriorityGroup = SDPG_World;
		RenderData->Draw_RenderThread(MeshBatch);

		Collector.AddMesh(ViewIndex, MeshBatch);
	}
}

FPrimitiveViewRelevance FVoxelMarchingCubeCollisionSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = true;
	Result.bRenderInMainPass = true;
	Result.bDynamicRelevance =
			View->Family->EngineShowFlags.Collision ||
			View->Family->EngineShowFlags.CollisionPawn ||
			View->Family->EngineShowFlags.CollisionVisibility;
	return Result;
}

uint32 FVoxelMarchingCubeCollisionSceneProxy::GetMemoryFootprint() const
{
	return sizeof(*this) + GetAllocatedSize();
}

SIZE_T FVoxelMarchingCubeCollisionSceneProxy::GetTypeHash() const
{
	static size_t UniquePointer;
	return reinterpret_cast<size_t>(&UniquePointer);
}