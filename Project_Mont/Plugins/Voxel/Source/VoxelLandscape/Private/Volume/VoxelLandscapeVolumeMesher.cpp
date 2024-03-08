// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/VoxelLandscapeVolumeMesher.h"
#include "Volume/VoxelLandscapeVolumeBrush.h"
#include "Volume/VoxelLandscapeVolumeProvider.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "Height/VoxelLandscapeHeightProvider.h"
#include "VoxelLandscapeState.h"
#include "VoxelLandscapeBrushManager.h"
#include "TransvoxelData.h"
#include "TransvoxelTransitionData.h"
#include "VoxelLandscapeVolumeMesherImpl.ispc.generated.h"

FVoxelLandscapeVolumeMesher::FVoxelLandscapeVolumeMesher(const FVoxelLandscapeChunkKey& ChunkKey, const TSharedRef<FVoxelLandscapeState>& State)
	: ChunkKey(ChunkKey)
	, State(State)
	, ChunkSize(State->ChunkSize)
	, DataSize(State->ChunkSize + 1)
	, MeshData(MakeVoxelShared<FVoxelLandscapeVolumeMeshData>(ChunkKey, *State))
{
	VOXEL_FUNCTION_COUNTER();

	const int32 EstimatedNumCells = 4 * ChunkSize * ChunkSize;

	MeshData->Cells.Reserve(EstimatedNumCells);
	MeshData->Indices.Reserve(12 * EstimatedNumCells);
	MeshData->Vertices.Reserve(4 * EstimatedNumCells);
	MeshData->TriangleToCellIndex.Reserve(4 * EstimatedNumCells);

	VertexIndexToCellIndex.Reserve(4 * EstimatedNumCells);
	CacheIndexToVertexIndex.Reserve(EstimatedNumCells);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFuture<FVoxelLandscapeVolumeMeshData> FVoxelLandscapeVolumeMesher::Build()
{
	VOXEL_FUNCTION_COUNTER();

	const int32 Step = 1 << ChunkKey.LOD;
	const FVoxelBox QueriedBounds = ChunkKey.GetQueriedBounds(*State);

	const TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> HeightBrushes = State->GetBrushes().GetHeightBrushes(FVoxelBox2D(QueriedBounds));
	const TVoxelArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> VolumeBrushes = State->GetBrushes().GetVolumeBrushes(QueriedBounds);

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
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeVolumeMesher::Generate()
{
	VOXEL_FUNCTION_COUNTER();
	check(Distances.Num() == FMath::Cube(DataSize));

	// Since we use SignBit below, -0 will lead to different results than +0
	// In practice it looks like a lot of math can converge to -0 (typically, a smooth union very far away from the object)
	// This is an issue because one chunk might get +0, the other -0, and neither of them will generate triangles
	FVoxelUtilities::FixupSignBit(Distances);

	FindCells();

	if (MeshData->Cells.Num() == 0)
	{
		return;
	}

	// Do this before ProcessCells to have all cells
	FindTransitionCells();

	ProcessCells();
	ProcessTransitionCells();
}

void FVoxelLandscapeVolumeMesher::FindCells()
{
	VOXEL_FUNCTION_COUNTER();
	using namespace Voxel;

	for (int32 Z = 0; Z < ChunkSize; Z++)
	{
		for (int32 Y = 0; Y < ChunkSize; Y++)
		{
			const int32 BaseIndex = FVoxelUtilities::Get3DIndex<int32>(DataSize, 0, Y, Z);
			for (int32 X = 0; X < ChunkSize; X++)
			{
				const int32 IndexOffset = BaseIndex + X;

#define INDEX(A, B, C) IndexOffset + A + B * DataSize + C * DataSize * DataSize

				checkVoxelSlow(INDEX(0, 0, 0) == GetIndex(X + 0, Y + 0, Z + 0));
				checkVoxelSlow(INDEX(1, 0, 0) == GetIndex(X + 1, Y + 0, Z + 0));
				checkVoxelSlow(INDEX(0, 1, 0) == GetIndex(X + 0, Y + 1, Z + 0));
				checkVoxelSlow(INDEX(1, 1, 0) == GetIndex(X + 1, Y + 1, Z + 0));
				checkVoxelSlow(INDEX(0, 0, 1) == GetIndex(X + 0, Y + 0, Z + 1));
				checkVoxelSlow(INDEX(1, 0, 1) == GetIndex(X + 1, Y + 0, Z + 1));
				checkVoxelSlow(INDEX(0, 1, 1) == GetIndex(X + 0, Y + 1, Z + 1));
				checkVoxelSlow(INDEX(1, 1, 1) == GetIndex(X + 1, Y + 1, Z + 1));

				int32 CellCode;
#if (PLATFORM_WINDOWS && !PLATFORM_COMPILER_CLANG) || PLATFORM_ALWAYS_HAS_AVX
				{
					const __m256 Distance = _mm256_setr_ps(
						Distances[INDEX(0, 0, 0)],
						Distances[INDEX(1, 0, 0)],
						Distances[INDEX(0, 1, 0)],
						Distances[INDEX(1, 1, 0)],
						Distances[INDEX(0, 0, 1)],
						Distances[INDEX(1, 0, 1)],
						Distances[INDEX(0, 1, 1)],
						Distances[INDEX(1, 1, 1)]);

					CellCode = _mm256_movemask_ps(Distance);
				}
#else
				{
					const float Distance0 = Distances[INDEX(0, 0, 0)];
					const float Distance1 = Distances[INDEX(1, 0, 0)];
					const float Distance2 = Distances[INDEX(0, 1, 0)];
					const float Distance3 = Distances[INDEX(1, 1, 0)];
					const float Distance4 = Distances[INDEX(0, 0, 1)];
					const float Distance5 = Distances[INDEX(1, 0, 1)];
					const float Distance6 = Distances[INDEX(0, 1, 1)];
					const float Distance7 = Distances[INDEX(1, 1, 1)];

					{
						// Most voxels are going to end up empty
						// We heavily optimize that hot path by just checking if they all have the same sign

						const bool bValue = FVoxelUtilities::SignBit(Distance0);
						if (bValue != FVoxelUtilities::SignBit(Distance1)) { goto Full; }
						if (bValue != FVoxelUtilities::SignBit(Distance2)) { goto Full; }
						if (bValue != FVoxelUtilities::SignBit(Distance3)) { goto Full; }
						if (bValue != FVoxelUtilities::SignBit(Distance4)) { goto Full; }
						if (bValue != FVoxelUtilities::SignBit(Distance5)) { goto Full; }
						if (bValue != FVoxelUtilities::SignBit(Distance6)) { goto Full; }
						if (bValue != FVoxelUtilities::SignBit(Distance7)) { goto Full; }
						continue;
					}

				Full:
					CellCode =
						(FVoxelUtilities::SignBit(Distance0) << 0) |
						(FVoxelUtilities::SignBit(Distance1) << 1) |
						(FVoxelUtilities::SignBit(Distance2) << 2) |
						(FVoxelUtilities::SignBit(Distance3) << 3) |
						(FVoxelUtilities::SignBit(Distance4) << 4) |
						(FVoxelUtilities::SignBit(Distance5) << 5) |
						(FVoxelUtilities::SignBit(Distance6) << 6) |
						(FVoxelUtilities::SignBit(Distance7) << 7);
				}
#endif

				if (CellCode == 0 ||
					CellCode == 255)
				{
					continue;
				}

				if (FVoxelUtilities::IntBits(Distances[INDEX(0, 0, 0)]) == 0xFFFFFFFF ||
					FVoxelUtilities::IntBits(Distances[INDEX(1, 0, 0)]) == 0xFFFFFFFF ||
					FVoxelUtilities::IntBits(Distances[INDEX(0, 1, 0)]) == 0xFFFFFFFF ||
					FVoxelUtilities::IntBits(Distances[INDEX(1, 1, 0)]) == 0xFFFFFFFF ||
					FVoxelUtilities::IntBits(Distances[INDEX(0, 0, 1)]) == 0xFFFFFFFF ||
					FVoxelUtilities::IntBits(Distances[INDEX(1, 0, 1)]) == 0xFFFFFFFF ||
					FVoxelUtilities::IntBits(Distances[INDEX(0, 1, 1)]) == 0xFFFFFFFF ||
					FVoxelUtilities::IntBits(Distances[INDEX(1, 1, 1)]) == 0xFFFFFFFF)
				{
					continue;
				}

#undef INDEX

				CellCode = ~CellCode & 0xFF;

				ensureVoxelSlow(FVoxelUtilities::IsValidUINT8(X));
				ensureVoxelSlow(FVoxelUtilities::IsValidUINT8(Y));
				ensureVoxelSlow(FVoxelUtilities::IsValidUINT8(Z));

				FCell Cell;
				Cell.X = uint8(X);
				Cell.Y = uint8(Y);
				Cell.Z = uint8(Z);
				Cell.FirstTriangle = CellCode;
				MeshData->Cells.Add(Cell);
			}
		}
	}
}

void FVoxelLandscapeVolumeMesher::ProcessCells()
{
	VOXEL_FUNCTION_COUNTER();
	using namespace Voxel;

	for (int32 CellIndex = 0; CellIndex < MeshData->Cells.Num(); CellIndex++)
	{
		FCell& Cell = MeshData->Cells[CellIndex];

		const int32 X = Cell.X;
		const int32 Y = Cell.Y;
		const int32 Z = Cell.Z;
		const int32 CellCode = Cell.FirstTriangle;

		checkVoxelSlow(CellCode != 0 && CellCode != 255);

		const int32 CellClass = Transvoxel::GetCellClass(CellCode);
		const Transvoxel::FCellIndices CellIndices = Transvoxel::CellClassToCellIndices[CellClass];
		const Transvoxel::FCellVertices CellVertices = Transvoxel::CellCodeToCellVertices[CellCode];

		// Indices of the vertices used in this cube
		TVoxelStaticArray<int32, 16> CellVertexIndices{ NoInit };
		for (int32 CellVertexIndex = 0; CellVertexIndex < CellVertices.NumVertices(); CellVertexIndex++)
		{
			const Transvoxel::FVertexData VertexData = CellVertices.GetVertexData(CellVertexIndex);

			// A: low point / B: high point
			const int32 IndexA = VertexData.IndexA;
			const int32 IndexB = VertexData.IndexB;
			checkVoxelSlow(0 <= IndexA && IndexA < 8);
			checkVoxelSlow(0 <= IndexB && IndexB < 8);

			const float DistanceA = Distances[GetIndex(X + bool(IndexA & 1), Y + bool(IndexA & 2), Z + bool(IndexA & 4))];
			const float DistanceB = Distances[GetIndex(X + bool(IndexB & 1), Y + bool(IndexB & 2), Z + bool(IndexB & 4))];
			ensureVoxelSlow(FVoxelUtilities::SignBit(DistanceA) != FVoxelUtilities::SignBit(DistanceB));

			const int32 EdgeIndex = VertexData.EdgeIndex;
			checkVoxelSlow(0 <= EdgeIndex && EdgeIndex < 3);

			const FIntVector PositionA(X + bool(IndexA & 1), Y + bool(IndexA & 2), Z + bool(IndexA & 4));
			const FIntVector PositionB(X + bool(IndexB & 1), Y + bool(IndexB & 2), Z + bool(IndexB & 4));

			const int32 CacheIndex = GetCacheIndex(PositionA, EdgeIndex);

			if (const int32* VertexIndex = CacheIndexToVertexIndex.Find(CacheIndex))
			{
				checkVoxelSlow(0 <= *VertexIndex && *VertexIndex < MeshData->Vertices.Num());
				CellVertexIndices[CellVertexIndex] = *VertexIndex;
				continue;
			}

			// Compute vertex

			const float Alpha = DistanceA / (DistanceA - DistanceB);

			if (VOXEL_DEBUG)
			{
				FIntVector Offset = FIntVector(ForceInit);
				Offset[EdgeIndex] = 1;
				ensure(PositionA + Offset == PositionB);
			}

			FVector3f Position = FVector3f(PositionA);
			Position[EdgeIndex] += Alpha;

			const int32 VertexIndexA = MeshData->Vertices.Add(Position);
			const int32 VertexIndexB = VertexIndexToCellIndex.Add(CellIndex);
			checkVoxelSlow(VertexIndexA == VertexIndexB);

			CacheIndexToVertexIndex.Add_CheckNew(CacheIndex, VertexIndexA);
			CellVertexIndices[CellVertexIndex] = VertexIndexA;
		}

		checkVoxelSlow(MeshData->Indices.Num() % 3 == 0);
		const int32 FirstTriangle = MeshData->Indices.Num() / 3;

		int32 NumValidTriangles = 0;
		for (int32 Index = 0; Index < CellIndices.NumTriangles(); Index++)
		{
			const int32 VertexIndex0 = CellVertexIndices[CellIndices.GetIndex(3 * Index + 0)];
			const int32 VertexIndex1 = CellVertexIndices[CellIndices.GetIndex(3 * Index + 1)];
			const int32 VertexIndex2 = CellVertexIndices[CellIndices.GetIndex(3 * Index + 2)];

			const FVector3f Vertex0 = MeshData->Vertices[VertexIndex0];
			const FVector3f Vertex1 = MeshData->Vertices[VertexIndex1];
			const FVector3f Vertex2 = MeshData->Vertices[VertexIndex2];

			if (!FVoxelUtilities::IsTriangleValid(Vertex0, Vertex1, Vertex2))
			{
				continue;
			}

			MeshData->Indices.Add(VertexIndex0);
			MeshData->Indices.Add(VertexIndex1);
			MeshData->Indices.Add(VertexIndex2);
			MeshData->TriangleToCellIndex.Add(CellIndex);

			NumValidTriangles++;
		}

		if (NumValidTriangles == 0)
		{
			MeshData->Cells.RemoveAtSwap(CellIndex);
			CellIndex--;
			continue;
		}

		ensureVoxelSlow(FVoxelUtilities::IsValidUINT8(NumValidTriangles));
		Cell.NumTriangles = uint8(NumValidTriangles);
		Cell.FirstTriangle = FirstTriangle;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeVolumeMesher::FindTransitionCells()
{
	VOXEL_FUNCTION_COUNTER();

	checkVoxelSlow(ChunkSize % 2 == 0);
	const int32 HalfChunkSize = ChunkSize / 2;

	for (int32 Index = 0; Index < 6; Index++)
	{
		TransitionCells[Index].SetNumZeroed(FMath::Square(HalfChunkSize));
	}

	for (const FCell& Cell : MeshData->Cells)
	{
		const uint32 X = Cell.X;
		const uint32 Y = Cell.Y;
		const uint32 Z = Cell.Z;

		if (X == 0) { TransitionCells[0][FVoxelUtilities::Get2DIndex<int32>(HalfChunkSize, Y / 2, Z / 2)] = true; }
		if (Y == 0) { TransitionCells[2][FVoxelUtilities::Get2DIndex<int32>(HalfChunkSize, Z / 2, X / 2)] = true; }
		if (Z == 0) { TransitionCells[4][FVoxelUtilities::Get2DIndex<int32>(HalfChunkSize, X / 2, Y / 2)] = true; }

		if (X == ChunkSize - 1) { TransitionCells[1][FVoxelUtilities::Get2DIndex<int32>(HalfChunkSize, Z / 2, Y / 2)] = true; }
		if (Y == ChunkSize - 1) { TransitionCells[3][FVoxelUtilities::Get2DIndex<int32>(HalfChunkSize, X / 2, Z / 2)] = true; }
		if (Z == ChunkSize - 1) { TransitionCells[5][FVoxelUtilities::Get2DIndex<int32>(HalfChunkSize, Y / 2, X / 2)] = true; }
	}
}

template<int32 Direction>
void FVoxelLandscapeVolumeMesher::ProcessTransitionCells()
{
	using namespace Voxel;

	checkVoxelSlow(ChunkSize % 2 == 0);
	const int32 HalfChunkSize = ChunkSize / 2;

	const int32 NumCells = TransitionCells[Direction].CountSetBits();

	TVoxelMap<int32, FTransitionIndex> CacheIndexToTransitionIndex;
	CacheIndexToTransitionIndex.Reserve(9 * NumCells);

	MeshData->TransitionIndices[Direction].Reserve(20 * NumCells);
	MeshData->TransitionVertices[Direction].Reserve(9 * NumCells);
	MeshData->TransitionTriangleToCellIndex[Direction].Reserve(7 * NumCells);

	TVoxelArray<FTransitionIndex> VertexIndices;
	VertexIndices.Reserve(32);

	TransitionCells[Direction].ForAllSetBits([&](const int32 BitIndex)
	{
		const int32 Y = BitIndex / HalfChunkSize;
		const int32 X = BitIndex - Y * HalfChunkSize;

		TVoxelStaticArray<float, 9> CornerValues{ NoInit };

		CornerValues[0] = Distances[GetIndex<Direction>(2 * X + 0, 2 * Y + 0)];
		CornerValues[1] = Distances[GetIndex<Direction>(2 * X + 1, 2 * Y + 0)];
		CornerValues[2] = Distances[GetIndex<Direction>(2 * X + 2, 2 * Y + 0)];
		CornerValues[3] = Distances[GetIndex<Direction>(2 * X + 0, 2 * Y + 1)];
		CornerValues[4] = Distances[GetIndex<Direction>(2 * X + 1, 2 * Y + 1)];
		CornerValues[5] = Distances[GetIndex<Direction>(2 * X + 2, 2 * Y + 1)];
		CornerValues[6] = Distances[GetIndex<Direction>(2 * X + 0, 2 * Y + 2)];
		CornerValues[7] = Distances[GetIndex<Direction>(2 * X + 1, 2 * Y + 2)];
		CornerValues[8] = Distances[GetIndex<Direction>(2 * X + 2, 2 * Y + 2)];

		int32 CaseCode =
			(FVoxelUtilities::SignBit(CornerValues[0]) << 0) |
			(FVoxelUtilities::SignBit(CornerValues[1]) << 1) |
			(FVoxelUtilities::SignBit(CornerValues[2]) << 2) |
			(FVoxelUtilities::SignBit(CornerValues[5]) << 3) |
			(FVoxelUtilities::SignBit(CornerValues[8]) << 4) |
			(FVoxelUtilities::SignBit(CornerValues[7]) << 5) |
			(FVoxelUtilities::SignBit(CornerValues[6]) << 6) |
			(FVoxelUtilities::SignBit(CornerValues[3]) << 7) |
			(FVoxelUtilities::SignBit(CornerValues[4]) << 8);

		CaseCode = ~CaseCode & 0x1FF;

		if (CaseCode == 0 ||
			CaseCode == 511)
		{
			return;
		}

		if (FVoxelUtilities::IntBits(CornerValues[0]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[1]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[2]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[5]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[8]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[7]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[6]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[3]) == 0xFFFFFFFF ||
			FVoxelUtilities::IntBits(CornerValues[4]) == 0xFFFFFFFF)
		{
			return;
		}

		checkVoxelSlow(0 <= CaseCode && CaseCode < 512);
		const Transvoxel::Transition::FCellClass CellClass = Transvoxel::Transition::CellCodeToCellClass[CaseCode];
		const Transvoxel::Transition::FVertexDatas& VertexDatas = Transvoxel::Transition::CellCodeToVertexDatas[CaseCode];
		const Transvoxel::Transition::FTransitionCellData& CellData = Transvoxel::Transition::CellClassToTransitionCellData[CellClass.Index];

		VertexIndices.Reset();
		for (int32 VertexDataIndex = 0; VertexDataIndex < CellData.NumVertices; VertexDataIndex++)
		{
			const Transvoxel::Transition::FVertexData VertexData = VertexDatas[VertexDataIndex];

			// A: low point / B: high point
			const int32 VertexIndexA = VertexData.IndexA;
			const int32 VertexIndexB = VertexData.GetIndexB();

			checkVoxelSlow(0 <= VertexIndexA && VertexIndexA < 13);
			checkVoxelSlow(0 <= VertexIndexB && VertexIndexB < 13);

			const auto GetVertexPosition = [&](const int32 VertexIndex)
			{
				switch (VertexIndex)
				{
				default: VOXEL_ASSUME(false);
				case 0: return GetPosition<Direction>(2 * X, 2 * Y);
				case 1: return GetPosition<Direction>(2 * X + 1, 2 * Y);
				case 2: return GetPosition<Direction>(2 * X + 2, 2 * Y);
				case 3: return GetPosition<Direction>(2 * X, 2 * Y + 1);
				case 4: return GetPosition<Direction>(2 * X + 1, 2 * Y + 1);
				case 5: return GetPosition<Direction>(2 * X + 2, 2 * Y + 1);
				case 6: return GetPosition<Direction>(2 * X, 2 * Y + 2);
				case 7: return GetPosition<Direction>(2 * X + 1, 2 * Y + 2);
				case 8: return GetPosition<Direction>(2 * X + 2, 2 * Y + 2);

				case 9: return GetPosition<Direction>(2 * X, 2 * Y);
				case 10: return GetPosition<Direction>(2 * X + 2, 2 * Y);
				case 11: return GetPosition<Direction>(2 * X, 2 * Y + 2);
				case 12: return GetPosition<Direction>(2 * X + 2, 2 * Y + 2);
				}
			};
			const auto GetVertexValue = [&](const int32 VertexIndex)
			{
				switch (VertexIndex)
				{
				default: VOXEL_ASSUME(false);
				case 0: return CornerValues[0];
				case 1: return CornerValues[1];
				case 2: return CornerValues[2];
				case 3: return CornerValues[3];
				case 4: return CornerValues[4];
				case 5: return CornerValues[5];
				case 6: return CornerValues[6];
				case 7: return CornerValues[7];
				case 8: return CornerValues[8];

				case 9: return CornerValues[0];
				case 10: return CornerValues[2];
				case 11: return CornerValues[6];
				case 12: return CornerValues[8];
				}
			};

			const FIntVector PositionA = GetVertexPosition(VertexIndexA);
			const FIntVector PositionB = GetVertexPosition(VertexIndexB);

			const bool bAlongX = PositionA.X != PositionB.X;
			const bool bAlongY = PositionA.Y != PositionB.Y;
			const bool bAlongZ = PositionA.Z != PositionB.Z;
			checkVoxelSlow(bAlongX + bAlongY + bAlongZ == 1);

			// Both vertices should be on the same side
			checkVoxelSlow((VertexIndexA < 9) == (VertexIndexB < 9));
			const bool bIsHighRes = VertexIndexA < 9;

			const int32 EdgeIndex = bAlongX ? 0 : bAlongY ? 1 : 2;
			{
				FIntVector NewPositionB = PositionA;
				NewPositionB[EdgeIndex] += bIsHighRes ? 1 : 2;
				checkVoxelSlow(NewPositionB == PositionB);
			}

			const int32 CacheIndex = GetCacheIndex(PositionA, EdgeIndex);

			if (bIsHighRes)
			{
				const int32* VertexIndexPtr = CacheIndexToVertexIndex.Find(CacheIndex);
				if (!ensure(VertexIndexPtr))
				{
					return;
				}

				VerticesToTranslate[*VertexIndexPtr] = true;

				FTransitionIndex TransitionIndex;
				TransitionIndex.bIsRelative = false;
				TransitionIndex.Index = *VertexIndexPtr;
				VertexIndices.Add(TransitionIndex);
				continue;
			}

			if (const FTransitionIndex* TransitionIndex = CacheIndexToTransitionIndex.Find(CacheIndex))
			{
				VertexIndices.Add(*TransitionIndex);
				continue;
			}

			int32 SourceVertex;
			if (const int32* SourceVertexPtr = CacheIndexToVertexIndex.Find(CacheIndex))
			{
				SourceVertex = *SourceVertexPtr;
			}
			else
			{
				FIntVector MiddlePosition = PositionA;
				MiddlePosition[EdgeIndex]++;
				SourceVertex = CacheIndexToVertexIndex.FindChecked(GetCacheIndex(MiddlePosition, EdgeIndex));
			}

			const float ValueA = GetVertexValue(VertexIndexA);
			const float ValueB = GetVertexValue(VertexIndexB);
			ensureVoxelSlow(FVoxelUtilities::SignBit(ValueA) != FVoxelUtilities::SignBit(ValueB));

			const float Alpha = ValueA / (ValueA - ValueB);

			FVector3f Position = FVector3f(PositionA);
			Position[EdgeIndex] += 2 * Alpha;

			const int32 Index = MeshData->TransitionVertices[Direction].Add({ Position, SourceVertex });

			FTransitionIndex TransitionIndex;
			TransitionIndex.bIsRelative = true;
			TransitionIndex.Index = Index;

			CacheIndexToTransitionIndex.Add_CheckNew(CacheIndex, TransitionIndex);
			VertexIndices.Add(TransitionIndex);
		}

		const int32 NumIndices = 3 * CellData.NumTriangles;
		for (int32 Index = 0; Index < NumIndices; Index += 3)
		{
			const FTransitionIndex IndexA = VertexIndices[CellData.Indices[Index + 0]];
			const FTransitionIndex IndexB = VertexIndices[CellData.Indices[Index + 1]];
			const FTransitionIndex IndexC = VertexIndices[CellData.Indices[Index + 2]];

			int32 CellIndex = 0;
			if (!IndexA.bIsRelative)
			{
				CellIndex = VertexIndexToCellIndex[IndexA.Index];
			}
			else if (!IndexB.bIsRelative)
			{
				CellIndex = VertexIndexToCellIndex[IndexB.Index];
			}
			else
			{
				ensureVoxelSlow(!IndexC.bIsRelative);
				CellIndex = VertexIndexToCellIndex[IndexC.Index];
			}

			// TODO Split up triangles spanning two cells and find exact cell overlapping triangle
			// Maybe floor/ceil vertices and find cells by position?

			if (CellClass.bIsInverted)
			{
				MeshData->TransitionIndices[Direction].Add(IndexA);
				MeshData->TransitionIndices[Direction].Add(IndexB);
				MeshData->TransitionIndices[Direction].Add(IndexC);
			}
			else
			{
				MeshData->TransitionIndices[Direction].Add(IndexC);
				MeshData->TransitionIndices[Direction].Add(IndexB);
				MeshData->TransitionIndices[Direction].Add(IndexA);
			}

			MeshData->TransitionTriangleToCellIndex[Direction].Add(CellIndex);
		}
	});
}

void FVoxelLandscapeVolumeMesher::ProcessTransitionCells()
{
	VOXEL_FUNCTION_COUNTER();

	VerticesToTranslate.SetNumZeroed(MeshData->Vertices.Num());

	ProcessTransitionCells<0>();
	ProcessTransitionCells<1>();
	ProcessTransitionCells<2>();
	ProcessTransitionCells<3>();
	ProcessTransitionCells<4>();
	ProcessTransitionCells<5>();

	TVoxelArray<int32> NewToOldIndices;
	FVoxelUtilities::SetNumFast(NewToOldIndices, MeshData->Vertices.Num());
	for (int32 Index = 0; Index < MeshData->Vertices.Num(); Index++)
	{
		NewToOldIndices[Index] = Index;
	}

	int32 FirstIndex = 0;
	VerticesToTranslate.ForAllSetBits([&](const int32 Index)
	{
		MeshData->Vertices.Swap(FirstIndex, Index);
		NewToOldIndices.Swap(FirstIndex, Index);
		FirstIndex++;
	});
	MeshData->NumEdgeVertices = FirstIndex;

	TVoxelArray<int32> OldToNewIndices;
	FVoxelUtilities::SetNumFast(OldToNewIndices, MeshData->Vertices.Num());
	for (int32 Index = 0; Index < MeshData->Vertices.Num(); Index++)
	{
		OldToNewIndices[NewToOldIndices[Index]] = Index;
	}

	for (int32& Index : MeshData->Indices)
	{
		Index = OldToNewIndices[Index];
	}

	for (TVoxelArray<FTransitionIndex>& Array : MeshData->TransitionIndices)
	{
		for (FTransitionIndex& TransitionIndex : Array)
		{
			if (TransitionIndex.bIsRelative)
			{
				continue;
			}

			TransitionIndex.Index = OldToNewIndices[TransitionIndex.Index];
		}
	}
	for (TVoxelArray<FTransitionVertex>& Array : MeshData->TransitionVertices)
	{
		for (FTransitionVertex& TransitionVertex : Array)
		{
			TransitionVertex.SourceVertex = OldToNewIndices[TransitionVertex.SourceVertex];
		}
	}
}