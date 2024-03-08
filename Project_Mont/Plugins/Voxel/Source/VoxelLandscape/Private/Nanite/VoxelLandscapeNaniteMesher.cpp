// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nanite/VoxelLandscapeNaniteMesher.h"
#include "Nanite/VoxelLandscapeNaniteMesh.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "Height/VoxelLandscapeHeightProvider.h"
#include "Height/VoxelLandscapeHeightUtilities.h"
#include "VoxelNaniteBuilder.h"
#include "VoxelLandscapeBrushManager.h"

TVoxelFuture<FVoxelLandscapeNaniteMesh> FVoxelLandscapeNaniteMesher::Build()
{
	VOXEL_FUNCTION_COUNTER();

	const int32 Step = 1 << ChunkKey.LOD;

	MinHeight = ChunkKey.GetChunkKeyBounds().Min.Z * State->ChunkSize * State->VoxelSize;
	MaxHeight = ChunkKey.GetChunkKeyBounds().Max.Z * State->ChunkSize * State->VoxelSize;

	const FVoxelBox2D QueriedBounds = ChunkKey.GetQueriedBounds2D(*State);

	const TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> HeightBrushes = State->GetBrushes().GetHeightBrushes(QueriedBounds);

	FVoxelUtilities::SetNumFast(Heights, HeightsSize * HeightsSize);
	FVoxelUtilities::SetAll(Heights, FVoxelUtilities::NaN());

	FVoxelFuture Future = FVoxelFuture::Done();
	for (const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& Brush : HeightBrushes)
	{
		const FVoxelLandscapeHeightQuery Query = Brush->MakeQuery(
			*State,
			State->ChunkSize * FVector2D(
				ChunkKey.ChunkKey.X,
				ChunkKey.ChunkKey.Y) - Step,
			Step,
			FIntPoint(HeightsSize),
			Heights);

		if (Query.Span.Area() == 0)
		{
			continue;
		}

		Future = Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
		{
			return Brush->HeightProvider->Apply(Query);
		}));
	}

	return Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
	{
		return Finalize();
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelLandscapeNaniteMesh> FVoxelLandscapeNaniteMesher::Finalize()
{
	VOXEL_FUNCTION_COUNTER();

	const int32 ChunkSizeWithPad = State->ChunkSize + 1;

	TVoxelArray<FVoxelOctahedron> Normals;
	FVoxelUtilities::SetNumFast(Normals, FMath::Square(ChunkSizeWithPad));

	FVoxelLandscapeHeightUtilities::ComputeNormals(
		Heights,
		HeightsSize,
		Normals,
		ChunkSizeWithPad,
		State->VoxelSize * (1 << ChunkKey.LOD));

	for (float& Height : Heights)
	{
		Height -= MinHeight;
	}
	const float Max = MaxHeight - MinHeight;

	for (int32 Y = 1; Y < HeightsSize - 1; Y++)
	{
		for (int32 X = 1; X < HeightsSize - 1; X++)
		{
#define CheckNeighbor(U, V) \
			{ \
				const float Height = Heights[FVoxelUtilities::Get2DIndex<int32>(HeightsSize, X + U, Y + V)]; \
				if (0 <= Height && Height <= Max) \
				{ \
					continue; \
				} \
			}

			CheckNeighbor(-1, -1);
			CheckNeighbor(-1, +0);
			CheckNeighbor(-1, +1);

			CheckNeighbor(+0, -1);
			CheckNeighbor(+0, +0);
			CheckNeighbor(+0, +1);

			CheckNeighbor(+1, -1);
			CheckNeighbor(+1, +0);
			CheckNeighbor(+1, +1);

#undef CheckNeighbor

			// No valid neighbor, don't need to render
			Heights[FVoxelUtilities::Get2DIndex<int32>(HeightsSize, X, Y)] = FVoxelUtilities::NaN();
		}
	}

	TVoxelArray<int32> VertexIndices;
	FVoxelUtilities::SetNumFast(VertexIndices, ChunkSizeWithPad * ChunkSizeWithPad);

	int32 NumVertices = 0;
	for (int32 Y = 1; Y < HeightsSize - 1; Y++)
	{
		for (int32 X = 1; X < HeightsSize - 1; X++)
		{
			const float Height = Heights[FVoxelUtilities::Get2DIndex<int32>(HeightsSize, X, Y)];
			const int32 Index = FVoxelUtilities::Get2DIndex<int32>(ChunkSizeWithPad, X - 1, Y - 1);

			if (FVoxelUtilities::IntBits(Height) == 0xFFFFFFFF)
			{
				VertexIndices[Index] = -1;
			}
			else
			{
				VertexIndices[Index] = NumVertices++;
			}
		}
	}

	const TSharedRef<FVoxelLandscapeNaniteMesh> Result = MakeVoxelShared<FVoxelLandscapeNaniteMesh>(ChunkKey);
	if (NumVertices == 0)
	{
		Result->bIsEmpty = true;
		return Result;
	}

	TVoxelArray<int32> Indices;
	Indices.Reserve(4 * NumVertices);
	for (int32 Y = 0; Y < State->ChunkSize; Y++)
	{
		for (int32 X = 0; X < State->ChunkSize; X++)
		{
			const int32 Index00 = VertexIndices[FVoxelUtilities::Get2DIndex<int32>(ChunkSizeWithPad, X + 0, Y + 0)];
			const int32 Index01 = VertexIndices[FVoxelUtilities::Get2DIndex<int32>(ChunkSizeWithPad, X + 1, Y + 0)];
			const int32 Index10 = VertexIndices[FVoxelUtilities::Get2DIndex<int32>(ChunkSizeWithPad, X + 0, Y + 1)];
			const int32 Index11 = VertexIndices[FVoxelUtilities::Get2DIndex<int32>(ChunkSizeWithPad, X + 1, Y + 1)];

			if (Index00 == -1 ||
				Index01 == -1 ||
				Index10 == -1 ||
				Index11 == -1)
			{
				continue;
			}

			Indices.Add(Index00);
			Indices.Add(Index10);
			Indices.Add(Index11);

			Indices.Add(Index00);
			Indices.Add(Index11);
			Indices.Add(Index01);
		}
	}

	if (Indices.Num() == 0)
	{
		Result->bIsEmpty = true;
		return Result;
	}

	const float ScaledVoxelSize = State->VoxelSize * (1 << ChunkKey.LOD);

	TVoxelArray<FVector3f> VertexPositions;
	TVoxelArray<FVoxelOctahedron> VertexNormals;
	TVoxelArray<FVector2f> TextureCoordinate;
	for (int32 Y = 0; Y < ChunkSizeWithPad; Y++)
	{
		for (int32 X = 0; X < ChunkSizeWithPad; X++)
		{
			const int32 Index = VertexIndices[FVoxelUtilities::Get2DIndex<int32>(ChunkSizeWithPad, X, Y)];
			if (Index == -1)
			{
				continue;
			}
			ensure(Index == VertexPositions.Num());

			const float Height = Heights[FVoxelUtilities::Get2DIndex<int32>(HeightsSize, X + 1, Y + 1)];
			const FVector3f Position = FVector3f(
				X,
				Y,
				Height / ScaledVoxelSize);

			VertexPositions.Add(Position);
			VertexNormals.Add(Normals[FVoxelUtilities::Get2DIndex<int32>(ChunkSizeWithPad, X, Y)]);
			TextureCoordinate.Add(FVector2f(
				X * (1 << ChunkKey.LOD),
				Y * (1 << ChunkKey.LOD)));
		}
	}

	FVoxelNaniteBuilder NaniteBuilder;
	NaniteBuilder.Mesh.Indices = Indices;
	NaniteBuilder.Mesh.Positions = VertexPositions;
	NaniteBuilder.Mesh.Normals = VertexNormals;
	NaniteBuilder.Mesh.TextureCoordinates.Add(TextureCoordinate);
	Result->RenderData = NaniteBuilder.CreateRenderData();;

	return Result;
}