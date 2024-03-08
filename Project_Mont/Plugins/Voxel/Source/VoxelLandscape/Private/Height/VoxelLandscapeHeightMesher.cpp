// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeHeightMesher.h"
#include "Height/VoxelLandscapeHeightMesh.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "Height/VoxelLandscapeHeightProvider.h"
#include "Height/VoxelLandscapeHeightUtilities.h"
#include "VoxelLandscapeBrushManager.h"

TVoxelFuture<FVoxelLandscapeHeightMesh> FVoxelLandscapeHeightMesher::Build()
{
	VOXEL_FUNCTION_COUNTER();

	const int32 Step = 1 << ChunkKey.LOD;

	MinHeight = ChunkKey.GetChunkKeyBounds().Min.Z * State->ChunkSize * State->VoxelSize;
	MaxHeight = ChunkKey.GetChunkKeyBounds().Max.Z * State->ChunkSize * State->VoxelSize;

	const FVoxelBox2D QueriedBounds = ChunkKey.GetQueriedBounds2D(*State);

	const TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> HeightBrushes = State->GetBrushes().GetHeightBrushes(QueriedBounds);

	FVoxelUtilities::SetNumFast(Heights, DataSize * DataSize);
	FVoxelUtilities::SetNumFast(DetailHeights, DetailDataSize * DetailDataSize);

	FVoxelUtilities::SetAll(Heights, FVoxelUtilities::NaN());
	FVoxelUtilities::SetAll(DetailHeights, FVoxelUtilities::NaN());

	FVoxelFuture Future = FVoxelFuture::Done();
	for (const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& Brush : HeightBrushes)
	{
		const FVoxelLandscapeHeightQuery Query = Brush->MakeQuery(
			*State,
			State->ChunkSize * FVector2D(
				ChunkKey.ChunkKey.X,
				ChunkKey.ChunkKey.Y),
			Step,
			FIntPoint(DataSize),
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

	Future = Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
	{
		FinalizeHeights();
	}));

	FVoxelFuture DetailFuture = FVoxelFuture::Done();
	for (const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& Brush : HeightBrushes)
	{
		const FVoxelLandscapeHeightQuery Query = Brush->MakeQuery(
			*State,
			State->ChunkSize * FVector2D(
				ChunkKey.ChunkKey.X,
				ChunkKey.ChunkKey.Y) - Step / double(State->DetailTextureSize),
			Step / double(State->DetailTextureSize),
			FIntPoint(DetailDataSize),
			DetailHeights);

		if (Query.Span.Area() == 0)
		{
			continue;
		}

		DetailFuture = DetailFuture.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
		{
			return Brush->HeightProvider->Apply(Query);
		}));
	}

	DetailFuture = DetailFuture.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
	{
		ComputeNormals();
	}));

	return Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
	{
		return DetailFuture.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
		{
			const TSharedRef<FVoxelLandscapeHeightMesh> Mesh = FVoxelLandscapeHeightMesh::New(
				ChunkKey,
				MoveTemp(Heights),
				MoveTemp(Normals));

			Mesh->Initialize_AsyncThread(*State);

			return Mesh;
		}));
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeHeightMesher::FinalizeHeights()
{
	VOXEL_FUNCTION_COUNTER();

	for (float& Height : Heights)
	{
		Height -= MinHeight;
	}
	const float Max = MaxHeight - MinHeight;

	for (int32 Y = 1; Y < DataSize - 1; Y++)
	{
		for (int32 X = 1; X < DataSize - 1; X++)
		{
#define CheckNeighbor(U, V) \
			{ \
				const float Height = Heights[FVoxelUtilities::Get2DIndex<int32>(DataSize, X + U, Y + V)]; \
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
			Heights[FVoxelUtilities::Get2DIndex<int32>(DataSize, X, Y)] = FVoxelUtilities::NaN();
		}
	}
}

void FVoxelLandscapeHeightMesher::ComputeNormals()
{
	VOXEL_FUNCTION_COUNTER();

	const int32 NormalSize = State->ChunkSize * State->DetailTextureSize;
	check(DetailDataSize == NormalSize + 2);

	FVoxelUtilities::SetNumFast(Normals, FMath::Square(NormalSize));

	FVoxelLandscapeHeightUtilities::ComputeNormals(
		DetailHeights,
		DetailDataSize,
		Normals,
		NormalSize,
		State->VoxelSize * (1 << ChunkKey.LOD) / double(State->DetailTextureSize));
}