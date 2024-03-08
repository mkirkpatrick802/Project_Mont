// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/VoxelLandscapeVolumeDetailTextureBuilder.h"
#include "Volume/VoxelLandscapeVolumeMeshData.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "Volume/VoxelLandscapeVolumeBrush.h"
#include "VoxelLandscapeState.h"
#include "VoxelLandscapeBrushManager.h"

FVoxelLandscapeVolumeDetailTextureBuilder::FVoxelLandscapeVolumeDetailTextureBuilder(
	const TSharedRef<FVoxelLandscapeState>& State,
	const TSharedRef<FVoxelLandscapeVolumeMeshData>& MeshData)
	: ChunkKey(MeshData->ChunkKey)
	, State(State)
	, MeshData(MeshData)
{
}

FVoxelFuture FVoxelLandscapeVolumeDetailTextureBuilder::Build()
{
	VOXEL_FUNCTION_COUNTER();

#if 0
	const int32 Step = 1 << ChunkKey.LOD;
	const FVoxelBox QueriedBounds = ChunkKey.GetQueriedBounds(*State);

	TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> HeightBrushes = State->GetBrushes().GetHeightBrushes(FVoxelBox2D(QueriedBounds));
	HeightBrushes.Sort([](const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& BrushA, const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& BrushB)
	{
		return BrushA->Priority < BrushB->Priority;
	});

	TVoxelArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> VolumeBrushes = State->GetBrushes().GetVolumeBrushes(QueriedBounds);
	VolumeBrushes.Sort([](const TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>& BrushA, const TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>& BrushB)
	{
		return BrushA->Priority < BrushB->Priority;
	});

	FVoxelUtilities::SetNumFast(Distances, FMath::Cube(DataSize));

	FVoxelFuture Future = FVoxelFuture::Done();

	if (HeightBrushes.Num() > 0)
	{
		VOXEL_SCOPE_COUNTER("Heights");

		FVoxelUtilities::SetNumFast(Heights, DataSize * DataSize);
		FVoxelUtilities::SetAll(Heights, FVoxelUtilities::NaN());

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

			if (Query.Span.Area() > 0)
			{
				Future = Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
				{
					return Brush->HeightProvider->Apply(Query);
				}));
			}
		}

		Future = Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
		{
			VOXEL_SCOPE_COUNTER_FORMAT("HeightToDistance Num=%d", FMath::Cube(DataSize));

			ispc::VoxelLandscapeVolumeMesher_HeightToDistance(
				DataSize,
				ChunkKey.ChunkKey.Z * State->ChunkSize * State->VoxelSize,
				Step * State->VoxelSize,
				Heights.GetData(),
				Distances.GetData());

			return FVoxelFuture::Done();
		}));
	}
	else
	{
		FVoxelUtilities::SetAll(Distances, FVoxelUtilities::NaN());
	}

	for (const TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>& Brush : VolumeBrushes)
	{
		const FVoxelBox BrushBounds = Brush->GetBounds(State->WorldToVoxel);
		const FIntVector Start = ChunkKey.ChunkKey * State->ChunkSize;

		FVoxelIntBox LocalBounds = FVoxelIntBox::FromFloatBox_WithPadding(BrushBounds)
			.ShiftBy(-Start)
			.DivideBigger(Step);

		LocalBounds.Min = FVoxelUtilities::ComponentMax(LocalBounds.Min, FIntVector(0));
		LocalBounds.Max = FVoxelUtilities::ComponentMin(LocalBounds.Max, FIntVector(DataSize));

		FVoxelLandscapeVolumeQuery Query;
		Query.Distances = Distances;
		Query.Span = LocalBounds;
		Query.StrideX = DataSize;
		Query.StrideXY = DataSize * DataSize;

		Query.IndexToBrush =
			FScaleMatrix(Step) *
			FTranslationMatrix(FVector(Start)) *
			State->WorldToVoxel.Inverse() *
			Brush->LocalToWorld.ToInverseMatrixWithScale();

		Query.DistanceScale = Brush->LocalToWorld.GetScale3D().GetAbsMax();

		Query.BlendMode = Brush->BlendMode;
		Query.Smoothness = Brush->Smoothness;

		Future = Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
		{
			return Brush->VolumeProvider->Apply(Query);
		}));
	}

	return Future.Then_VoxelThread(MakeStrongPtrLambda(this, [=]
	{
		Generate();
		MeshData->Shrink();
		return MeshData;
	}));
#endif

	return {};
}