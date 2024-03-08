// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_GenerateSurfacePoints2D.h"
#include "VoxelRuntime.h"
#include "VoxelPositionQueryParameter.h"
#include "VoxelNode_GenerateSurfacePoints2DImpl.ispc.generated.h"

void FVoxelNode_Generate2DPoints::GeneratePositions(
	const FVoxelBox& Bounds,
	const int32 NumCellsX,
	const int32 NumCellsY,
	const float CellSize,
	const float Jitter,
	const FVoxelSeed& Seed,
	FVoxelVectorBuffer& OutPositions,
	FVoxelPointIdBuffer& OutIds)
{
	VOXEL_FUNCTION_COUNTER_NUM(NumCellsX * NumCellsY, 32);

	FVoxelPointIdBufferStorage IdStorage;
	FVoxelFloatBufferStorage PositionsX;
	FVoxelFloatBufferStorage PositionsY;

	IdStorage.Allocate(NumCellsX * NumCellsY);
	PositionsX.Allocate(NumCellsX * NumCellsY);
	PositionsY.Allocate(NumCellsX * NumCellsY);

	{
		VOXEL_SCOPE_COUNTER("Compute positions");

		const float MinX = Bounds.Min.X;
		const float MinY = Bounds.Min.Y;
		const float RelativeJitter = CellSize * Jitter;
		const FVoxelPointRandom RandomX(Seed, STATIC_HASH("GenerateSurfacePoints2D_JitterX"));
		const FVoxelPointRandom RandomY(Seed, STATIC_HASH("GenerateSurfacePoints2D_JitterY"));

		const uint64 BaseHash = FVoxelUtilities::MurmurHashMulti(
			FVoxelUtilities::RoundToInt32(Bounds.Min.X),
			FVoxelUtilities::RoundToInt32(Bounds.Min.Y),
			FVoxelUtilities::RoundToInt32(Bounds.Min.Z));

		ForeachVoxelBufferChunk_Parallel(NumCellsX * NumCellsY, [&](const FVoxelBufferIterator& Iterator)
		{
			const TVoxelArrayView<FVoxelPointId> IdsView = IdStorage.GetRawView_NotConstant(Iterator);
			const TVoxelArrayView<float> PositionsXView = PositionsX.GetRawView_NotConstant(Iterator);
			const TVoxelArrayView<float> PositionsYView = PositionsY.GetRawView_NotConstant(Iterator);

			const int32 BaseIndex = Iterator.GetIndex();
			for (int32 Index = 0; Index < Iterator.Num(); Index++)
			{
				const int32 CurrentIndex = BaseIndex + Index;
				const int32 IndexY = CurrentIndex / NumCellsX;
				const int32 IndexX = CurrentIndex - IndexY * NumCellsX;
				checkVoxelSlow(CurrentIndex == FVoxelUtilities::Get2DIndex<int32>(NumCellsX, NumCellsY, IndexX, IndexY));

				const float PositionX = MinX + IndexX * CellSize;
				const float PositionY = MinY + IndexY * CellSize;

				const FVoxelPointId Id = FVoxelUtilities::MurmurHashMulti(PositionX, PositionY) ^ BaseHash;

				IdsView[Index] = Id;

				PositionsXView[Index] = PositionX + RelativeJitter * (2.f * RandomX.GetFraction(Id) - 1.f);
				PositionsYView[Index] = PositionY + RelativeJitter * (2.f * RandomY.GetFraction(Id) - 1.f);
			}
		});
	}

	OutIds = FVoxelPointIdBuffer::Make(IdStorage);

	FVoxelFloatBufferStorage PositionsZ;
	PositionsZ.AddZeroed(NumCellsX * NumCellsY);
	OutPositions = FVoxelVectorBuffer::Make(PositionsX, PositionsY, PositionsZ);
}

FVoxelVectorBuffer FVoxelNode_Generate2DPoints::GenerateGradientPositions(
	const FVoxelVectorBuffer& Positions,
	const float CellSize) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num() * 4, 32);

	const int32 GradientNum = Positions.Num() * 4;

	FVoxelFloatBufferStorage PositionX;
	FVoxelFloatBufferStorage PositionY;

	PositionX.Allocate(GradientNum);
	PositionY.Allocate(GradientNum);

	struct FGradient : TVoxelBufferMultiIteratorType<4> {};
	struct FInput : TVoxelBufferMultiIteratorType<1> {};
	
	for (TVoxelBufferMultiIterator<FGradient, FInput> Iterator(GradientNum, Positions.Num()); Iterator; ++Iterator)
	{
		ispc::VoxelNode_GenerateSurfacePoints2D_SplitPositions(
			Positions.X.GetData(Iterator.Get<FInput>()),
			Positions.Y.GetData(Iterator.Get<FInput>()),
			Iterator.Num<FGradient>(),
			CellSize / 2.f,
			PositionX.GetData(Iterator.Get<FGradient>()),
			PositionY.GetData(Iterator.Get<FGradient>()));
	}

	FVoxelFloatBufferStorage PositionZ;
	PositionZ.AddZeroed(GradientNum);

	return FVoxelVectorBuffer::Make(PositionX, PositionY, PositionZ);
}

FVoxelVectorBuffer FVoxelNode_Generate2DPoints::ComputeGradient(
	const FVoxelFloatBuffer& Heights,
	const float CellSize) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Heights.Num(), 32);

	if (Heights.IsConstant())
	{
		return FVoxelVectorBuffer::Make(FVector3f::UpVector);
	}

	check(Heights.Num() % 4 == 0);
	const int32 InputNum = Heights.Num() / 4;

	FVoxelFloatBufferStorage NormalX;
	FVoxelFloatBufferStorage NormalY;
	FVoxelFloatBufferStorage NormalZ;

	NormalX.Allocate(InputNum);
	NormalY.Allocate(InputNum);
	NormalZ.Allocate(InputNum);

	struct FGradient : TVoxelBufferMultiIteratorType<4> {};
	struct FInput : TVoxelBufferMultiIteratorType<1> {};
	
	for (TVoxelBufferMultiIterator<FGradient, FInput> Iterator(Heights.Num(), InputNum); Iterator; ++Iterator)
	{
		ispc::VoxelNode_GenerateSurfacePoints2D_Collapse(
			Heights.GetData(Iterator.Get<FGradient>()),
			Iterator.Num<FInput>(),
			CellSize,
			NormalX.GetData(Iterator.Get<FInput>()),
			NormalY.GetData(Iterator.Get<FInput>()),
			NormalZ.GetData(Iterator.Get<FInput>()));
	}

	return FVoxelVectorBuffer::Make(NormalX, NormalY, NormalZ);
}

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_Generate2DPoints, Out)
{
	const TValue<FVoxelBox> Bounds = Get(BoundsPin, Query);
	const TValue<float> CellSize = Get(CellSizePin, Query);
	const TValue<float> Jitter = Get(JitterPin, Query);
	const TValue<FVoxelSeed> Seed = Get(SeedPin, Query);

	return VOXEL_ON_COMPLETE(Bounds, CellSize, Jitter, Seed)
	{
		if (!Bounds.IsValid() ||
			Bounds == FVoxelBox())
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid bounds", this);
			return {};
		}
		if (Bounds.IsInfinite())
		{
			VOXEL_MESSAGE(Error, "{0}: Infinite bounds", this);
			return {};
		}

		const int32 NumCellsX = (Bounds.Max.X - Bounds.Min.X) / CellSize;
		const int32 NumCellsY = (Bounds.Max.Y - Bounds.Min.Y) / CellSize;

		FVoxelNodeStatScope StatScope(*this, NumCellsX * NumCellsY);

		if (NumCellsX * NumCellsY == 0)
		{
			return {};
		}

		FVoxelVectorBuffer Positions;
		FVoxelPointIdBuffer Ids;
		GeneratePositions(
			Bounds,
			NumCellsX,
			NumCellsY,
			CellSize,
			Jitter,
			Seed,
			Positions,
			Ids);

		TValue<FVoxelFloatBuffer> Heights;
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelPositionQueryParameter>().Initialize(Positions);
			Parameters->Add<FVoxelGradientStepQueryParameter>().Step = CellSize;

			Heights = Get(HeightPin, Query.MakeNewQuery(Parameters));
		}

		return VOXEL_ON_COMPLETE(CellSize, Ids, Positions, Heights, Bounds)
		{
			CheckVoxelBuffersNum(Positions, Heights);

			FVoxelPointIdBuffer FilteredIds;
			FVoxelVectorBuffer FilteredPositions;
			if (Heights.IsConstant())
			{
				if (Heights.GetConstant() < Bounds.Min.Z ||
					Heights.GetConstant() > Bounds.Max.Z)
				{
					return {};
				}

				FilteredIds = Ids;

				FilteredPositions.X = Positions.X;
				FilteredPositions.Y = Positions.Y;

				FVoxelFloatBufferStorage NewPositionsZ;
				NewPositionsZ.BulkAdd(Heights.GetConstant(), Positions.Num());
				FilteredPositions.Z = FVoxelFloatBuffer::Make(NewPositionsZ);
			}
			else
			{
				FVoxelPointIdBufferStorage NewIds;
				FVoxelFloatBufferStorage NewPositionsX;
				FVoxelFloatBufferStorage NewPositionsY;
				FVoxelFloatBufferStorage NewPositionsZ;

				ForeachVoxelBufferChunk_Sync("Filter heights", Heights.Num(), [&](const FVoxelBufferIterator& Iterator)
				{
					const TConstVoxelArrayView<FVoxelPointId> IdsView = Ids.GetRawView_NotConstant(Iterator);
					const TConstVoxelArrayView<float> PositionsXView = Positions.X.GetRawView_NotConstant(Iterator);
					const TConstVoxelArrayView<float> PositionsYView = Positions.Y.GetRawView_NotConstant(Iterator);
					const TConstVoxelArrayView<float> HeightsView = Heights.GetRawView_NotConstant(Iterator);
					for (int32 Index = 0; Index < HeightsView.Num(); Index++)
					{
						const float Height = HeightsView[Index];
						if (Height < Bounds.Min.Z ||
							Height > Bounds.Max.Z)
						{
							continue;
						}

						NewIds.Add(IdsView[Index]);
						NewPositionsX.Add(PositionsXView[Index]);
						NewPositionsY.Add(PositionsYView[Index]);
						NewPositionsZ.Add(Height);
					}
				});

				FilteredIds = FVoxelPointIdBuffer::Make(NewIds);

				FilteredPositions = FVoxelVectorBuffer::Make(
					NewPositionsX,
					NewPositionsY,
					NewPositionsZ);
			}

			if (FilteredPositions.Num() == 0)
			{
				return {};
			}

			const FVoxelVectorBuffer GradientPositions = GenerateGradientPositions(FilteredPositions, CellSize);

			const TValue<FVoxelFloatBuffer> GradientHeights = INLINE_LAMBDA
			{
				const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
				Parameters->Add<FVoxelPositionQueryParameter>().Initialize(GradientPositions);
				Parameters->Add<FVoxelGradientStepQueryParameter>().Step = CellSize;

				return Get(HeightPin, Query.MakeNewQuery(Parameters));
			};

			return VOXEL_ON_COMPLETE(CellSize, FilteredIds, FilteredPositions, GradientPositions, GradientHeights)
			{
				CheckVoxelBuffersNum(GradientPositions, GradientHeights);

				const TSharedRef<FVoxelPointSet> PointSet = MakeVoxelShared<FVoxelPointSet>();
				PointSet->SetNum(FilteredPositions.Num());
				PointSet->Add(FVoxelPointAttributes::Id, FilteredIds);
				PointSet->Add(FVoxelPointAttributes::Position, FilteredPositions);
				PointSet->Add(FVoxelPointAttributes::Normal, ComputeGradient(GradientHeights, CellSize));
				return PointSet;
			};
		};
	};
}