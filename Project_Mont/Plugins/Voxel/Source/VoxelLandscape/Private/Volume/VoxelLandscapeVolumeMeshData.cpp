// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/VoxelLandscapeVolumeMeshData.h"
#include "VoxelLandscapeState.h"
#include "Volume/VoxelLandscapeVolumeMesh.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelLandscapeVolumeMeshDataMemory);

FVoxelLandscapeVolumeMeshData::FVoxelLandscapeVolumeMeshData(
	const FVoxelLandscapeChunkKey& ChunkKey,
	const FVoxelLandscapeState& State)
	: ChunkKey(ChunkKey)
	, ChunkSize(State.ChunkSize)
{
}

int64 FVoxelLandscapeVolumeMeshData::GetAllocatedSize() const
{
	int64 AllocatedSize = 0;

	AllocatedSize += Cells.GetAllocatedSize();
	AllocatedSize += TriangleToCellIndex.GetAllocatedSize();
	AllocatedSize += CellIndexToDirection.GetAllocatedSize();
	AllocatedSize += CellIndexToTextureCoordinate.GetAllocatedSize();

	AllocatedSize += Indices.GetAllocatedSize();
	AllocatedSize += Vertices.GetAllocatedSize();
	AllocatedSize += VertexNormals.GetAllocatedSize();

	for (const TVoxelArray<FTransitionIndex>& Array : TransitionIndices)
	{
		AllocatedSize += Array.GetAllocatedSize();
	}
	for (const TVoxelArray<FTransitionVertex>& Array : TransitionVertices)
	{
		AllocatedSize += Array.GetAllocatedSize();
	}
	for (const TVoxelArray<int32>& Array : TransitionTriangleToCellIndex)
	{
		AllocatedSize += Array.GetAllocatedSize();
	}

	return AllocatedSize;
}

void FVoxelLandscapeVolumeMeshData::Shrink()
{
	VOXEL_FUNCTION_COUNTER();

	Cells.Shrink();
	TriangleToCellIndex.Shrink();
	CellIndexToDirection.Shrink();
	CellIndexToTextureCoordinate.Shrink();

	Indices.Shrink();
	Vertices.Shrink();
	VertexNormals.Shrink();

	for (TVoxelArray<FTransitionIndex>& Array : TransitionIndices)
	{
		Array.Shrink();
	}
	for (TVoxelArray<FTransitionVertex>& Array : TransitionVertices)
	{
		Array.Shrink();
	}
	for (TVoxelArray<int32>& Array : TransitionTriangleToCellIndex)
	{
		Array.Shrink();
	}
}

TSharedRef<FVoxelLandscapeVolumeMesh> FVoxelLandscapeVolumeMeshData::CreateMesh(const uint8 TransitionMask) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(Cells.Num() > 0);

	TVoxelArray<int32> NewIndices;
	TVoxelArray<FVector3f> NewVertices;
	TVoxelArray<int32> NewTriangleToCellIndex;
	TVoxelArray<FVector3f> NewVertexNormals;
	{
		int32 NumIndices = 0;
		int32 NumVertices = 0;
		for (int32 Direction = 0; Direction < 6; Direction++)
		{
			if (!(TransitionMask & (1 << Direction)))
			{
				continue;
			}

			NumIndices += TransitionIndices[Direction].Num();
			NumVertices += TransitionVertices[Direction].Num();
		}
		NewIndices.Reserve(NumIndices);
		NewVertices.Reserve(2 * NumVertices);

		ensure(NumIndices % 3 == 0);
		NewTriangleToCellIndex.Reserve(NumIndices / 3);

		if (VertexNormals.Num() > 0)
		{
			NewVertexNormals.Reserve(2 * NumVertices);
		}
	}

	NewIndices.Append(Indices);
	NewVertices.Append(Vertices);
	NewTriangleToCellIndex.Append(TriangleToCellIndex);

	if (VertexNormals.Num() > 0)
	{
		NewVertexNormals.Append(VertexNormals);
	}

	for (int32 Direction = 0; Direction < 6; Direction++)
	{
		if (!(TransitionMask & (1 << Direction)))
		{
			continue;
		}

		const int32 Offset = NewVertices.Num();

		for (const FTransitionIndex& TransitionIndex : TransitionIndices[Direction])
		{
			int32 Index = TransitionIndex.Index;
			if (TransitionIndex.bIsRelative)
			{
				Index += Offset;
			}
			NewIndices.Add(Index);
		}

		for (const FTransitionVertex& TransitionVertex : TransitionVertices[Direction])
		{
			NewVertices.Add(TransitionVertex.Position);
		}

		NewTriangleToCellIndex.Append(TransitionTriangleToCellIndex[Direction]);

		if (VertexNormals.Num() > 0)
		{
			for (const FTransitionVertex& TransitionVertex : TransitionVertices[Direction])
			{
				NewVertexNormals.Add(VertexNormals[TransitionVertex.SourceVertex]);
			}
		}
	}

	if (TransitionMask != 0)
	{
		VOXEL_SCOPE_COUNTER("Translate vertices");

		const float LowerBound = 1;
		const float UpperBound = ChunkSize - 1;

		ensure(VertexNormals.Num() == NumEdgeVertices || VertexNormals.Num() == Vertices.Num());
		for (int32 Index = 0; Index < NumEdgeVertices; Index++)
		{
			FVector3f& Vertex = NewVertices[Index];

			if ((LowerBound <= Vertex.X && Vertex.X <= UpperBound) &&
				(LowerBound <= Vertex.Y && Vertex.Y <= UpperBound) &&
				(LowerBound <= Vertex.Z && Vertex.Z <= UpperBound))
			{
				// Fast exit
				continue;
			}

			constexpr uint8 XMin = 0x01;
			constexpr uint8 XMax = 0x02;
			constexpr uint8 YMin = 0x04;
			constexpr uint8 YMax = 0x08;
			constexpr uint8 ZMin = 0x10;
			constexpr uint8 ZMax = 0x20;

			if ((Vertex.X == 0.f && !(TransitionMask & XMin)) || (Vertex.X == ChunkSize && !(TransitionMask & XMax)) ||
				(Vertex.Y == 0.f && !(TransitionMask & YMin)) || (Vertex.Y == ChunkSize && !(TransitionMask & YMax)) ||
				(Vertex.Z == 0.f && !(TransitionMask & ZMin)) || (Vertex.Z == ChunkSize && !(TransitionMask & ZMax)))
			{
				// Can't translate when on a corner
				continue;
			}

			FVector3f Delta(0.f);

			if ((TransitionMask & XMin) && Vertex.X < LowerBound)
			{
				Delta.X = LowerBound - Vertex.X;
			}
			if ((TransitionMask & XMax) && Vertex.X > UpperBound)
			{
				Delta.X = UpperBound - Vertex.X;
			}
			if ((TransitionMask & YMin) && Vertex.Y < LowerBound)
			{
				Delta.Y = LowerBound - Vertex.Y;
			}
			if ((TransitionMask & YMax) && Vertex.Y > UpperBound)
			{
				Delta.Y = UpperBound - Vertex.Y;
			}
			if ((TransitionMask & ZMin) && Vertex.Z < LowerBound)
			{
				Delta.Z = LowerBound - Vertex.Z;
			}
			if ((TransitionMask & ZMax) && Vertex.Z > UpperBound)
			{
				Delta.Z = UpperBound - Vertex.Z;
			}

			Delta /= 4;

			const FVector3f Normal = VertexNormals[Index];
			ensureVoxelSlow(Normal.IsNormalized() || Normal.IsZero());

			Vertex += FVector3f(
				(1 - Normal.X * Normal.X) * Delta.X - Normal.Y * Normal.X * Delta.Y - Normal.Z * Normal.X * Delta.Z,
				-Normal.X * Normal.Y * Delta.X + (1 - Normal.Y * Normal.Y) * Delta.Y - Normal.Z * Normal.Y * Delta.Z,
				-Normal.X * Normal.Z * Delta.X - Normal.Y * Normal.Z * Delta.Y + (1 - Normal.Z * Normal.Z) * Delta.Z);
		}
	}

	struct FPrimitiveData
	{
		union
		{
			struct
			{
				uint32 DetailTextureU : 15;
				uint32 DetailTextureV : 15;
				uint32 Direction : 2;
			};
			uint32 Word;
		};
	};
	checkStatic(sizeof(FPrimitiveData) == sizeof(uint32));
	TVoxelArray<FPrimitiveData> PrimitiveDatas;
	{
		VOXEL_SCOPE_COUNTER("Build PrimitiveData");
		ensure(3 * NewTriangleToCellIndex.Num() == NewIndices.Num());

		PrimitiveDatas.Reserve(2 * NewVertices.Num());
		FVoxelUtilities::SetNumFast(PrimitiveDatas, NewVertices.Num());
		FVoxelUtilities::Memset(PrimitiveDatas, 0xFF);

		for (int32 TriangleIndex = 0; TriangleIndex < NewTriangleToCellIndex.Num(); TriangleIndex++)
		{
			const int32 CellIndex = NewTriangleToCellIndex[TriangleIndex];
			if (!ensureVoxelSlow(CellIndexToDirection.IsValidIndex(CellIndex)))
			{
				continue;
			}

			const FTextureCoordinate TextureCoordinate = CellIndexToTextureCoordinate[CellIndex];
			checkVoxelSlow(TextureCoordinate.X < (1 << 15));
			checkVoxelSlow(TextureCoordinate.Y < (1 << 15));

			FPrimitiveData PrimitiveData;
			PrimitiveData.DetailTextureU = TextureCoordinate.X;
			PrimitiveData.DetailTextureV = TextureCoordinate.Y;
			PrimitiveData.Direction = CellIndexToDirection[CellIndex];

			const int32 IndexA = NewIndices[3 * TriangleIndex + 0];
			const int32 IndexB = NewIndices[3 * TriangleIndex + 1];
			const int32 IndexC = NewIndices[3 * TriangleIndex + 2];

			// Set the data on the leading vertex
			// This will be the value used by nointerpolation attributes

			if (PrimitiveDatas[IndexA].Word == -1 ||
				PrimitiveDatas[IndexA].Word == PrimitiveData.Word)
			{
				PrimitiveDatas[IndexA] = PrimitiveData;
				continue;
			}
			if (PrimitiveDatas[IndexB].Word == -1 ||
				PrimitiveDatas[IndexB].Word == PrimitiveData.Word)
			{
				NewIndices[3 * TriangleIndex + 0] = IndexB;
				NewIndices[3 * TriangleIndex + 1] = IndexC;
				NewIndices[3 * TriangleIndex + 2] = IndexA;
				PrimitiveDatas[IndexB] = PrimitiveData;
				continue;
			}
			if (PrimitiveDatas[IndexC].Word == -1 ||
				PrimitiveDatas[IndexC].Word == PrimitiveData.Word)
			{
				NewIndices[3 * TriangleIndex + 0] = IndexC;
				NewIndices[3 * TriangleIndex + 1] = IndexA;
				NewIndices[3 * TriangleIndex + 2] = IndexB;
				PrimitiveDatas[IndexC] = PrimitiveData;
				continue;
			}

			const int32 NewIndex0 = NewVertices.Add(MakeCopy(NewVertices[IndexA]));
			const int32 NewIndex1 = PrimitiveDatas.Add(PrimitiveData);
			checkVoxelSlow(NewIndex0 == NewIndex1);

			if (VertexNormals.Num() > 0)
			{
				const int32 NewIndex2 = NewVertexNormals.Add(MakeCopy(NewVertexNormals[IndexA]));
				checkVoxelSlow(NewIndex0 == NewIndex2);
			}

			NewIndices[3 * TriangleIndex + 0] = NewIndex0;
		}

		// Make sure all PrimitiveData are valid for vertex shader detail textures
		for (int32 TriangleIndex = 0; TriangleIndex < NewTriangleToCellIndex.Num(); TriangleIndex++)
		{
			const int32 CellIndex = NewTriangleToCellIndex[TriangleIndex];
			if (!ensureVoxelSlow(CellIndexToDirection.IsValidIndex(CellIndex)))
			{
				continue;
			}

			const FTextureCoordinate TextureCoordinate = CellIndexToTextureCoordinate[CellIndex];
			checkVoxelSlow(TextureCoordinate.X < (1 << 15));
			checkVoxelSlow(TextureCoordinate.Y < (1 << 15));

			FPrimitiveData PrimitiveData;
			PrimitiveData.DetailTextureU = TextureCoordinate.X;
			PrimitiveData.DetailTextureV = TextureCoordinate.Y;
			PrimitiveData.Direction = CellIndexToDirection[CellIndex];

			const int32 IndexA = NewIndices[3 * TriangleIndex + 0];
			const int32 IndexB = NewIndices[3 * TriangleIndex + 1];
			const int32 IndexC = NewIndices[3 * TriangleIndex + 2];

			if (PrimitiveDatas[IndexA].Word == -1)
			{
				PrimitiveDatas[IndexA] = PrimitiveData;
			}
			if (PrimitiveDatas[IndexB].Word == -1)
			{
				PrimitiveDatas[IndexB] = PrimitiveData;
			}
			if (PrimitiveDatas[IndexC].Word == -1)
			{
				PrimitiveDatas[IndexC] = PrimitiveData;
			}
		}
	}

	TVoxelArray<FVector4f> FinalVertices;
	check(NewVertices.Num() == PrimitiveDatas.Num());

	FVoxelUtilities::SetNumFast(FinalVertices, NewVertices.Num());

	for (int32 Index = 0; Index < NewVertices.Num(); Index++)
	{
		const FVector3f& Vertex = NewVertices[Index];

		FinalVertices[Index] = FVector4f(
			Vertex.X,
			Vertex.Y,
			Vertex.Z,
			FVoxelUtilities::FloatBits(PrimitiveDatas[Index].Word));
	}

	// TODO Vertex normals
	return FVoxelLandscapeVolumeMesh::New(
		ChunkKey,
		MoveTemp(NewIndices),
		MoveTemp(FinalVertices),
		{});
}