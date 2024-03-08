// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/VoxelLandscapeVolumeMesh.h"
#include "Volume/VoxelLandscapeVolumeVertexFactory.h"
#include "VoxelLandscapeState.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelLandscapeVolumeMeshGpuMemory);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelLandscapeVolumeMesh> FVoxelLandscapeVolumeMesh::New(
	const FVoxelLandscapeChunkKey& ChunkKey,
	TVoxelArray<int32>&& Indices,
	TVoxelArray<FVector4f>&& Vertices,
	TVoxelArray<FVector3f>&& VertexNormals)
{
	const TSharedRef<FVoxelLandscapeVolumeMesh> Mesh = MakeVoxelShareable_RenderThread(new (GVoxelMemory) FVoxelLandscapeVolumeMesh(ChunkKey));
	Mesh->Bounds = FVoxelBox::FromPositions(Vertices);
	Mesh->Indices = MoveTemp(Indices);
	Mesh->Vertices = MoveTemp(Vertices);
	Mesh->VertexNormals = MoveTemp(VertexNormals);
	return Mesh;
}

FVoxelLandscapeVolumeMesh::~FVoxelLandscapeVolumeMesh()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	if (IndicesBuffer)
	{
		IndicesBuffer->ReleaseResource();
		IndicesBuffer.Reset();
	}
	if (VerticesBuffer)
	{
		VerticesBuffer->ReleaseResource();
		VerticesBuffer.Reset();
	}
	if (VertexNormalsBuffer)
	{
		VertexNormalsBuffer->ReleaseResource();
		VertexNormalsBuffer.Reset();
	}
	if (VertexFactory)
	{
		VertexFactory->ReleaseResource();
		VertexFactory.Reset();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int64 FVoxelLandscapeVolumeMesh::GetGpuAllocatedSize() const
{
	int64 AllocatedSize = 0;
	if (IndicesBuffer &&
		IndicesBuffer->IndexBufferRHI)
	{
		AllocatedSize += IndicesBuffer->IndexBufferRHI->GetSize();
	}
	if (VerticesBuffer &&
		VerticesBuffer->VertexBufferRHI)
	{
		AllocatedSize += VerticesBuffer->VertexBufferRHI->GetSize();
	}
	if (VertexNormalsBuffer &&
		VertexNormalsBuffer->VertexBufferRHI)
	{
		AllocatedSize += VertexNormalsBuffer->VertexBufferRHI->GetSize();
	}
	return AllocatedSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeVolumeMesh::Initialize_RenderThread(
	FRHICommandListBase& RHICmdList,
	const FVoxelLandscapeState& State)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());
	ON_SCOPE_EXIT
	{
		UpdateGpuStats();
	};

	Material = State.Material;
	Bounds = Bounds.Scale(State.VoxelSize * (1 << ChunkKey.LOD));

	NumIndices = Indices.Num();

	IndicesBuffer = MakeVoxelShared<FIndexBuffer>();
	{
		VOXEL_SCOPE_COUNTER("Indices");

		FVoxelResourceArrayRef ResourceArray(Indices);
		FRHIResourceCreateInfo CreateInfo(TEXT("Indices"), &ResourceArray);
		IndicesBuffer->IndexBufferRHI = UE_503_SWITCH(RHICreateIndexBuffer, RHICmdList.CreateIndexBuffer)(
			sizeof(uint32),
			Indices.Num() * sizeof(uint32),
			BUF_Static,
			CreateInfo);
		IndicesBuffer->InitResource(UE_503_ONLY(RHICmdList));

		Indices.Empty();
	}

	VerticesBuffer = MakeVoxelShared<FVertexBuffer>();
	{
		VOXEL_SCOPE_COUNTER("Vertices");

		FVoxelResourceArrayRef ResourceArray(Vertices);
		FRHIResourceCreateInfo CreateInfo(TEXT("Vertices"), &ResourceArray);
		VerticesBuffer->VertexBufferRHI = UE_503_SWITCH(RHICreateVertexBuffer, RHICmdList.CreateVertexBuffer)(
			Vertices.Num() * sizeof(FVector4f),
			BUF_Static | BUF_ShaderResource,
			CreateInfo);

		VerticesBuffer->InitResource(UE_503_ONLY(RHICmdList));

		Vertices.Empty();
	}

	if (VertexNormals.Num() > 0)
	{
		VOXEL_SCOPE_COUNTER("VertexNormals");

		VertexNormalsBuffer = MakeVoxelShared<FVertexBuffer>();
		{
			FVoxelResourceArrayRef ResourceArray(VertexNormals);
			FRHIResourceCreateInfo CreateInfo(TEXT("VertexNormals"), &ResourceArray);
			VertexNormalsBuffer->VertexBufferRHI = UE_503_SWITCH(RHICreateVertexBuffer, RHICmdList.CreateVertexBuffer)(
				VertexNormals.Num() * sizeof(FVector3f),
				BUF_Static | BUF_ShaderResource,
				CreateInfo);

			VertexNormalsBuffer->InitResource(UE_503_ONLY(RHICmdList));

			VertexNormals.Empty();
		}
	}

	if (VertexNormalsBuffer.IsValid())
	{
		VertexFactory = MakeVoxelShared<FVoxelLandscapeVolumeVertexFactory_WithVertexNormals>(GMaxRHIFeatureLevel);
	}
	else
	{
		VertexFactory = MakeVoxelShared<FVoxelLandscapeVolumeVertexFactory_NoVertexNormals>(GMaxRHIFeatureLevel);
	}

	VertexFactory->ScaledVoxelSize = State.VoxelSize * (1 << ChunkKey.LOD);
	VertexFactory->RawVoxelSize = State.VoxelSize;

	VertexFactory->PositionAndPrimitiveDataComponent = FVertexStreamComponent(
		VerticesBuffer.Get(),
		0,
		sizeof(FVector4f),
		VET_Float4);

	if (VertexNormalsBuffer.IsValid())
	{
		VertexFactory->VertexNormalComponent = FVertexStreamComponent(
			VertexNormalsBuffer.Get(),
			0,
			sizeof(FVector3f),
			VET_Float3);
	}

	VOXEL_INLINE_COUNTER("VertexFactory InitResource", VertexFactory->InitResource(UE_503_ONLY(RHICmdList)));
}