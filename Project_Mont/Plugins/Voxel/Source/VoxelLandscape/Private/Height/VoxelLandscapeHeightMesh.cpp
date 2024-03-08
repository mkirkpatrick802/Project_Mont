// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeHeightMesh.h"
#include "Height/VoxelLandscapeHeightVertexFactory.h"
#include "VoxelLandscapeState.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelLandscapeHeightMeshMemory);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelLandscapeHeightMeshGpuMemory);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelLandscapeHeightMesh> FVoxelLandscapeHeightMesh::New(
	const FVoxelLandscapeChunkKey& ChunkKey,
	TVoxelArray<float>&& Heights,
	TVoxelArray<FVoxelOctahedron>&& Normals)
{
	const TSharedRef<FVoxelLandscapeHeightMesh> Mesh = MakeVoxelShareable_RenderThread(new (GVoxelMemory) FVoxelLandscapeHeightMesh(ChunkKey));
	Mesh->Heights = MoveTemp(Heights);
	Mesh->Normals = MoveTemp(Normals);
	return Mesh;
}

FVoxelLandscapeHeightMesh::~FVoxelLandscapeHeightMesh()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());

	if (IndicesBuffer)
	{
		IndicesBuffer->ReleaseResource();
		IndicesBuffer.Reset();
	}

	if (HeightsBuffer)
	{
		HeightsBuffer->Release();
		HeightsBuffer.Reset();
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

int64 FVoxelLandscapeHeightMesh::GetAllocatedSize() const
{
	return
	Indices.GetAllocatedSize() +
		Heights.GetAllocatedSize() +
		Normals.GetAllocatedSize();
}

int64 FVoxelLandscapeHeightMesh::GetGpuAllocatedSize() const
{
	int64 AllocatedSize = 0;

	if (IndicesBuffer &&
		IndicesBuffer->IndexBufferRHI)
	{
		AllocatedSize += IndicesBuffer->IndexBufferRHI->GetSize();
	}

	if (HeightsBuffer)
	{
		AllocatedSize += HeightsBuffer->NumBytes;
	}

	if (NormalTexture)
	{
		AllocatedSize += NormalTexture->GetDesc().CalcMemorySizeEstimate();
	}

	return AllocatedSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeHeightMesh::Initialize_AsyncThread(const FVoxelLandscapeState& State)
{
	VOXEL_FUNCTION_COUNTER();
	ON_SCOPE_EXIT
	{
		UpdateStats();
		UpdateGpuStats();
	};

	Material = State.Material;

	const int32 DataSize = State.ChunkSize + 1;
	check(Heights.Num() == DataSize * DataSize);

	TVoxelArray<int32> OldToNewHeights;
	{
		VOXEL_SCOPE_COUNTER("Compact heights");

		OldToNewHeights.Reserve(Heights.Num());

		int32 Index = 0;
		for (const float Height : Heights)
		{
			if (FVoxelUtilities::IntBits(Height) == 0xFFFFFFFF)
			{
				OldToNewHeights.Add(-1);
				continue;
			}
			checkVoxelSlow(!FVoxelUtilities::IsNaN(Height));

			const int32 NewIndex = Index++;
			Heights[NewIndex] = Height;
			OldToNewHeights.Add(NewIndex);
		}
		Heights.SetNum(Index, false);
	}

	if (Heights.Num() == 0)
	{
		return;
	}

	Indices.Reserve(State.ChunkSize * State.ChunkSize * 3 * 2);

	Bounds = FVoxelBox::InvertedInfinite;

	const float ScaledVoxelSize = State.VoxelSize * (1 << ChunkKey.LOD);

	for (int32 QuadY = 0; QuadY < State.ChunkSize; QuadY++)
	{
		for (int32 QuadX = 0; QuadX < State.ChunkSize; QuadX++)
		{
			const int32 HeightIndex00 = OldToNewHeights[FVoxelUtilities::Get2DIndex<int32>(DataSize, QuadX + 0, QuadY + 0)];
			const int32 HeightIndex01 = OldToNewHeights[FVoxelUtilities::Get2DIndex<int32>(DataSize, QuadX + 1, QuadY + 0)];
			const int32 HeightIndex10 = OldToNewHeights[FVoxelUtilities::Get2DIndex<int32>(DataSize, QuadX + 0, QuadY + 1)];
			const int32 HeightIndex11 = OldToNewHeights[FVoxelUtilities::Get2DIndex<int32>(DataSize, QuadX + 1, QuadY + 1)];

			if (HeightIndex00 == -1 ||
				HeightIndex01 == -1 ||
				HeightIndex10 == -1 ||
				HeightIndex11 == -1)
			{
				continue;
			}

			Bounds += FVector((QuadX + 0) * ScaledVoxelSize, (QuadY + 0) * ScaledVoxelSize, Heights[HeightIndex00]);
			Bounds += FVector((QuadX + 1) * ScaledVoxelSize, (QuadY + 0) * ScaledVoxelSize, Heights[HeightIndex01]);
			Bounds += FVector((QuadX + 0) * ScaledVoxelSize, (QuadY + 1) * ScaledVoxelSize, Heights[HeightIndex10]);
			Bounds += FVector((QuadX + 1) * ScaledVoxelSize, (QuadY + 1) * ScaledVoxelSize, Heights[HeightIndex11]);

			checkVoxelSlow(FVoxelUtilities::IsValidUINT8(QuadX + 0));
			checkVoxelSlow(FVoxelUtilities::IsValidUINT8(QuadX + 1));
			checkVoxelSlow(FVoxelUtilities::IsValidUINT8(QuadY + 0));
			checkVoxelSlow(FVoxelUtilities::IsValidUINT8(QuadY + 1));

			checkVoxelSlow(FVoxelUtilities::IsValidUINT16(HeightIndex00));
			checkVoxelSlow(FVoxelUtilities::IsValidUINT16(HeightIndex01));
			checkVoxelSlow(FVoxelUtilities::IsValidUINT16(HeightIndex10));
			checkVoxelSlow(FVoxelUtilities::IsValidUINT16(HeightIndex11));

			const uint32 PackedIndex00 = (uint32(QuadX + 0) << 24) | (uint32(QuadY + 0) << 16) | uint32(HeightIndex00);
			const uint32 PackedIndex01 = (uint32(QuadX + 1) << 24) | (uint32(QuadY + 0) << 16) | uint32(HeightIndex01);
			const uint32 PackedIndex10 = (uint32(QuadX + 0) << 24) | (uint32(QuadY + 1) << 16) | uint32(HeightIndex10);
			const uint32 PackedIndex11 = (uint32(QuadX + 1) << 24) | (uint32(QuadY + 1) << 16) | uint32(HeightIndex11);

			Indices.Add(PackedIndex00);
			Indices.Add(PackedIndex10);
			Indices.Add(PackedIndex11);

			Indices.Add(PackedIndex00);
			Indices.Add(PackedIndex11);
			Indices.Add(PackedIndex01);
		}
	}

	NumIndices = Indices.Num();

	{
		VOXEL_SCOPE_COUNTER("RHIAsyncCreateTexture2D");

		const int32 Size = State.ChunkSize * State.DetailTextureSize;
		check(Normals.Num() == FMath::Square(Size));

		TArray<void*> MipData;
		MipData.Add(Normals.GetData());

		FGraphEventRef CompletionEvent;
		NormalTexture = RHIAsyncCreateTexture2D(
			Size,
			Size,
			// PF_R16_UINT so we can use GatherRed
			PF_R16_UINT,
			1,
			TexCreate_ShaderResource,
			MipData.GetData(),
			1
			UE_503_ONLY(, CompletionEvent));

		if (CompletionEvent.IsValid())
		{
			VOXEL_SCOPE_COUNTER("Wait");
			CompletionEvent->Wait();
		}

		if (!ensure(NormalTexture))
		{
			NumIndices = 0;
			return;
		}

		NormalTexture->SetName("VoxelHeightNormal");
		Normals.Empty();
	}
}

void FVoxelLandscapeHeightMesh::Initialize_RenderThread(
	FRHICommandListBase& RHICmdList,
	const FVoxelLandscapeState& State)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInRenderingThread());
	check(NumIndices > 0);
	ON_SCOPE_EXIT
	{
		UpdateStats();
		UpdateGpuStats();
	};

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

	HeightsBuffer = MakeVoxelShared<FReadBuffer>();
	{
		VOXEL_SCOPE_COUNTER("Heights");

		FVoxelResourceArrayRef ResourceArray(Heights);
		HeightsBuffer->Initialize(
			TEXT("Heights"),
			sizeof(float),
			Heights.Num(),
			PF_R32_FLOAT,
			EBufferUsageFlags::None,
			&ResourceArray);

		Heights.Empty();
	}

	VertexFactory = MakeVoxelShared<FVoxelLandscapeHeightVertexFactory>(GMaxRHIFeatureLevel);
	VertexFactory->ChunkSize = State.ChunkSize;
	VertexFactory->ScaledVoxelSize = State.VoxelSize * (1 << ChunkKey.LOD);
	VertexFactory->RawVoxelSize = State.VoxelSize;
	VertexFactory->Heights = HeightsBuffer->SRV;
	VertexFactory->NormalTexture = NormalTexture;

	VOXEL_INLINE_COUNTER("VertexFactory InitResource", VertexFactory->InitResource(UE_503_ONLY(RHICmdList)));
}