// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPositionQueryParameter.h"
#include "VoxelPositionQueryParameterImpl.ispc.generated.h"

FVoxelBox FVoxelPositionQueryParameter::GetBounds() const
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	return GetBounds_RequiresLock();
}

FVoxelVectorBuffer FVoxelPositionQueryParameter::GetPositions() const
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	return GetPositions_RequiresLock();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelPositionQueryParameterHelper
{
public:
	using FCell = FVoxelPositionQueryParameter::FCell;

	const FVoxelBox BoundsToFilter;
	const FIntVector Min;
	const FIntVector Max;
	const FIntVector GridSize;
	const TVoxelBufferStorage<FCell>& Cells;
	const FVoxelInt32BufferStorage& Indices;
	const FVoxelFloatBufferStorage& PositionsX;
	const FVoxelFloatBufferStorage& PositionsY;
	const FVoxelFloatBufferStorage& PositionsZ;
	FVoxelInt32BufferStorage& OutIndices;
	FVoxelFloatBufferStorage& OutPositionsX;
	FVoxelFloatBufferStorage& OutPositionsY;
	FVoxelFloatBufferStorage& OutPositionsZ;

public:
	void Filter() const
	{
		VOXEL_FUNCTION_COUNTER();
		checkVoxelSlow(PositionsX.Num() == PositionsY.Num());
		checkVoxelSlow(PositionsX.Num() == PositionsZ.Num());
		const int32 NumCells = (Max.X - Min.X) * (Max.Y - Min.Y) * (Max.Z - Min.Z);

		int32 MaxNum = 0;
		TVoxelArray<FCell> CellsToProcess;
		{
			VOXEL_SCOPE_COUNTER_FORMAT("FindCells NumCells=%d", NumCells);
			CellsToProcess.Reserve(NumCells);

			for (int32 X = Min.X; X < Max.X; X++)
			{
				for (int32 Y = Min.Y; Y < Max.Y; Y++)
				{
					for (int32 Z = Min.Z; Z < Max.Z; Z++)
					{
						const int32 GridIndex = FVoxelUtilities::Get3DIndex<int32>(GridSize, X, Y, Z);
						const FCell Cell = Cells.LoadFast(GridIndex);
						if (Cell.Num == 0)
						{
							continue;
						}

						MaxNum += Cell.Num;
						CellsToProcess.Add_NoGrow(Cell);
					}
				}
			}
		}

		OutIndices.SetNumUninitialized(MaxNum);
		OutPositionsX.SetNumUninitialized(MaxNum);
		OutPositionsY.SetNumUninitialized(MaxNum);
		OutPositionsZ.SetNumUninitialized(MaxNum);

		int32 Num = 0;
		{
			VOXEL_SCOPE_COUNTER_FORMAT("ProcessCells Num=%d", MaxNum);

			for (const FCell Cell : CellsToProcess)
			{
				FVoxelBufferIterator Iterator;
				Iterator.Initialize(Cell.Index + Cell.Num, Cell.Index);
				for (; Iterator; ++Iterator)
				{
					const TConstVoxelArrayView<int32> IndicesView = Indices.GetRawView_NotConstant(Iterator);
					FilterCell(Num, IndicesView);
				}
			}
		}

		VOXEL_SCOPE_COUNTER_FORMAT("%d checked/%d valid", MaxNum, Num);

		OutIndices.SetNumUninitialized(Num);
		OutPositionsX.SetNumUninitialized(Num);
		OutPositionsY.SetNumUninitialized(Num);
		OutPositionsZ.SetNumUninitialized(Num);
	}

private:
	FORCENOINLINE void FilterCell(
		int32& NumRef,
		const TConstVoxelArrayView<int32> IndicesView) const
	{
		int32 Num = NumRef;
		ON_SCOPE_EXIT
		{
			NumRef = Num;
		};

		const float MinX = float(BoundsToFilter.Min.X) - UE_KINDA_SMALL_NUMBER;
		const float MinY = float(BoundsToFilter.Min.Y) - UE_KINDA_SMALL_NUMBER;
		const float MinZ = float(BoundsToFilter.Min.Z) - UE_KINDA_SMALL_NUMBER;

		const float MaxX = float(BoundsToFilter.Max.X) + UE_KINDA_SMALL_NUMBER;
		const float MaxY = float(BoundsToFilter.Max.Y) + UE_KINDA_SMALL_NUMBER;
		const float MaxZ = float(BoundsToFilter.Max.Z) + UE_KINDA_SMALL_NUMBER;

		const float* RESTRICT const* RESTRICT PositionsXPtr = PositionsX.GetISPCBuffer();
		const float* RESTRICT const* RESTRICT PositionsYPtr = PositionsY.GetISPCBuffer();
		const float* RESTRICT const* RESTRICT PositionsZPtr = PositionsZ.GetISPCBuffer();

		int32* RESTRICT OutIndicesPtr = &OutIndices[Num];
		float* RESTRICT OutPositionsXPtr = &OutPositionsX[Num];
		float* RESTRICT OutPositionsYPtr = &OutPositionsY[Num];
		float* RESTRICT OutPositionsZPtr = &OutPositionsZ[Num];
		int32 WriteIndex = 0;

		for (const int32 PositionIndex : IndicesView)
		{
			const int32 ChunkIndex = FVoxelBufferDefinitions::GetChunkIndex(PositionIndex);
			const int32 ChunkOffset = FVoxelBufferDefinitions::GetChunkOffset(PositionIndex);

			const float PositionX = PositionsXPtr[ChunkIndex][ChunkOffset];
			if (PositionX < MinX ||
				PositionX > MaxX)
			{
				continue;
			}

			const float PositionY = PositionsYPtr[ChunkIndex][ChunkOffset];
			if (PositionY < MinY ||
				PositionY > MaxY)
			{
				continue;
			}

			const float PositionZ = PositionsZPtr[ChunkIndex][ChunkOffset];
			if (PositionZ < MinZ ||
				PositionZ > MaxZ)
			{
				continue;
			}

			checkVoxelSlow(&OutIndicesPtr[WriteIndex] == &OutIndices[Num]);
			checkVoxelSlow(&OutPositionsXPtr[WriteIndex] == &OutPositionsX[Num]);
			checkVoxelSlow(&OutPositionsYPtr[WriteIndex] == &OutPositionsY[Num]);
			checkVoxelSlow(&OutPositionsZPtr[WriteIndex] == &OutPositionsZ[Num]);

			OutIndicesPtr[WriteIndex] = PositionIndex;
			OutPositionsXPtr[WriteIndex] = PositionX;
			OutPositionsYPtr[WriteIndex] = PositionY;
			OutPositionsZPtr[WriteIndex] = PositionZ;

			WriteIndex++;
			Num++;

			if (Num % FVoxelBufferDefinitions::NumPerChunk != 0)
			{
				continue;
			}

			if (Num == OutIndices.Num())
			{
				checkVoxelSlow(Num == OutPositionsX.Num());
				checkVoxelSlow(Num == OutPositionsY.Num());
				checkVoxelSlow(Num == OutPositionsZ.Num());
				checkVoxelSlow(PositionIndex == IndicesView.Last());
				continue;
			}

			OutIndicesPtr = &OutIndices[Num];
			OutPositionsXPtr = &OutPositionsX[Num];
			OutPositionsYPtr = &OutPositionsY[Num];
			OutPositionsZPtr = &OutPositionsZ[Num];
			WriteIndex = 0;
		}
	}
};

bool FVoxelPositionQueryParameter::TryFilter(
	const FVoxelBox& BoundsToFilter,
	FVoxelInt32Buffer& OutIndices,
	FVoxelVectorBuffer& OutPositions) const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	const FVoxelBox Bounds = GetBounds_RequiresLock();
	if (!ensureVoxelSlow(BoundsToFilter.Intersect(Bounds)))
	{
		OutIndices = FVoxelInt32Buffer::MakeEmpty();
		OutPositions = FVoxelVectorBuffer::MakeEmpty();
		return true;
	}

	if (BoundsToFilter.Contains(Bounds))
	{
		// No need to filter
		return false;
	}

	FVoxelInt32BufferStorage Indices;
	FVoxelFloatBufferStorage PositionsX;
	FVoxelFloatBufferStorage PositionsY;
	FVoxelFloatBufferStorage PositionsZ;
	ON_SCOPE_EXIT
	{
		OutIndices = FVoxelInt32Buffer::Make(Indices);
		OutPositions = FVoxelVectorBuffer::Make(
			PositionsX,
			PositionsY,
			PositionsZ);
	};

	const FVoxelVectorBuffer Positions = GetPositions_RequiresLock();
	if (Positions.Num() < 32)
	{
		for (int32 Index = 0; Index < Positions.Num(); Index++)
		{
			const FVector3f Position = Positions[Index];
			if (!BoundsToFilter.Contains(Position))
			{
				continue;
			}

			Indices.Add(Index);
			PositionsX.Add(Position.X);
			PositionsY.Add(Position.Y);
			PositionsZ.Add(Position.Z);
		}
		return true;
	}

	const FCells& Cells = GetCells_RequiresLock();

	FVoxelBox LocalBounds = BoundsToFilter.Overlap(Bounds).ShiftBy(-Cells.Offset);
	LocalBounds /= Cells.Step;

	const FIntVector Min = FVoxelUtilities::Clamp(FVoxelUtilities::FloorToInt(LocalBounds.Min), 0, Cells.GridSize);
	const FIntVector Max = FVoxelUtilities::Clamp(FVoxelUtilities::CeilToInt(LocalBounds.Max), 0, Cells.GridSize);

	FVoxelPositionQueryParameterHelper Helper
	{
		BoundsToFilter,
		Min,
		Max,
		Cells.GridSize,
		ReinterpretCastRef<TVoxelBufferStorage<FCell>>(Cells.Cells.GetStorage()),
		Cells.Indices.GetStorage(),
		Positions.X.GetStorage(),
		Positions.Y.GetStorage(),
		Positions.Z.GetStorage(),
		Indices,
		PositionsX,
		PositionsY,
		PositionsZ
	};

	Helper.Filter();

	// Might raise incorrectly due to over-careful bound check above
	if (VOXEL_DEBUG && false)
	{
		TVoxelSet<int32> ExpectedIndices;
		ExpectedIndices.Reserve(Indices.Num());
		for (int32 Index = 0; Index < Positions.Num(); Index++)
		{
			if (!BoundsToFilter.Contains(Positions[Index]))
			{
				continue;
			}

			ExpectedIndices.Add(Index);
		}

		TVoxelSet<int32> ActualIndices;
		ActualIndices.Reserve(Indices.Num());
		for (const int32 Index : Indices)
		{
			ActualIndices.Add(Index);
		}

		ensure(ExpectedIndices.OrderIndependentEqual(ActualIndices));
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelPositionQueryParameter::Initialize(
	const FVoxelVectorBuffer& NewPositions,
	const TOptional<FVoxelBox>& NewBounds)
{
	ensure(NewPositions.Num() > 0);

	PrecomputedBounds = NewBounds;
	Compute = MakeVoxelShared<TVoxelUniqueFunction<FVoxelVectorBuffer()>>([NewPositions]
	{
		return NewPositions;
	});

	if (VOXEL_DEBUG && NewBounds)
	{
		CheckBounds();
	}
}

void FVoxelPositionQueryParameter::Initialize(
	TVoxelUniqueFunction<FVoxelVectorBuffer()>&& NewCompute,
	const TOptional<FVoxelBox>& NewBounds)
{
	PrecomputedBounds = NewBounds;
	Compute = MakeSharedCopy(MoveTemp(NewCompute));

	if (VOXEL_DEBUG && NewBounds)
	{
		CheckBounds();
	}
}

void FVoxelPositionQueryParameter::InitializeGradient(
	const FVoxelVectorBuffer& NewPositions,
	const TOptional<FVoxelBox>& NewBounds)
{
	ensure(NewPositions.Num() > 0);

	bIsGradient = true;
	PrecomputedBounds = NewBounds;
	Compute = MakeVoxelShared<TVoxelUniqueFunction<FVoxelVectorBuffer()>>([NewPositions]
	{
		return NewPositions;
	});

	if (VOXEL_DEBUG && NewBounds)
	{
		CheckBounds();
	}
}

void FVoxelPositionQueryParameter::InitializeGrid(
	const FVector3f& Start,
	const float Step,
	const FIntVector& Size)
{
	if (!ensure(int64(Size.X) * int64(Size.Y) * int64(Size.Z) < MAX_int32))
	{
		return;
	}

	Grid = MakeVoxelShared<FGrid>(FGrid
	{
		Start,
		Step,
		Size
	});

	Initialize(
		[=]
		{
			VOXEL_SCOPE_COUNTER("VoxelPositionQueryParameter_WritePositions3D");
			VOXEL_SCOPE_COUNTER_FORMAT("Size = %dx%dx%d", Size.X, Size.Y, Size.Z);

			const int32 Num = Size.X * Size.Y * Size.Z;

			FVoxelFloatBufferStorage X; X.Allocate(Num);
			FVoxelFloatBufferStorage Y;	Y.Allocate(Num);
			FVoxelFloatBufferStorage Z;	Z.Allocate(Num);

			ForeachVoxelBufferChunk_Parallel(Num, [&](const FVoxelBufferIterator& Iterator)
			{
				ispc::VoxelPositionQueryParameter_WritePositions3D(
					X.GetData(Iterator),
					Y.GetData(Iterator),
					Z.GetData(Iterator),
					Start.X,
					Start.Y,
					Start.Z,
					Size.X,
					Size.Y,
					Step,
					Iterator.GetIndex(),
					Iterator.Num());
			});

			return FVoxelVectorBuffer::Make(X, Y, Z);
		},
		FVoxelBox(FVector(Start), FVector(Start) + Step * FVector(Size)));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelPositionQueryParameter::CheckBounds() const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelVectorBuffer Positions = GetPositions();
	const FVoxelBox Bounds = GetBounds();

	for (int32 Index = 0; Index < Positions.Num(); Index++)
	{
		const FVector3f Position = Positions[Index];

		ensure(Bounds.Min.X - 1 <= Position.X);
		ensure(Bounds.Min.Y - 1 <= Position.Y);
		ensure(Bounds.Min.Z - 1 <= Position.Z);

		ensure(Position.X <= Bounds.Max.X + 1);
		ensure(Position.Y <= Bounds.Max.Y + 1);
		ensure(Position.Z <= Bounds.Max.Z + 1);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelBox FVoxelPositionQueryParameter::GetBounds_RequiresLock() const
{
	if (CachedBounds_RequiresLock)
	{
		return *CachedBounds_RequiresLock;
	}

	if (PrecomputedBounds)
	{
		CachedBounds_RequiresLock = PrecomputedBounds.GetValue();
		return *CachedBounds_RequiresLock;
	}

	if (!CachedPositions_RequiresLock)
	{
		VOXEL_SCOPE_COUNTER("Compute positions");
		CachedPositions_RequiresLock = (*Compute)();
		ensure(CachedPositions_RequiresLock->Num() > 0);
	}

	VOXEL_SCOPE_COUNTER("Compute bounds");

	const FFloatInterval MinMaxX = CachedPositions_RequiresLock->X.GetStorage().GetMinMaxSafe();
	const FFloatInterval MinMaxY = CachedPositions_RequiresLock->Y.GetStorage().GetMinMaxSafe();
	const FFloatInterval MinMaxZ = CachedPositions_RequiresLock->Z.GetStorage().GetMinMaxSafe();

	CachedBounds_RequiresLock = FVoxelBox(
		FVector(MinMaxX.Min, MinMaxY.Min, MinMaxZ.Min),
		FVector(MinMaxX.Max, MinMaxY.Max, MinMaxZ.Max));

	return *CachedBounds_RequiresLock;
}

FVoxelVectorBuffer FVoxelPositionQueryParameter::GetPositions_RequiresLock() const
{
	if (CachedPositions_RequiresLock)
	{
		return *CachedPositions_RequiresLock;
	}

	VOXEL_SCOPE_COUNTER("Compute positions");

	CachedPositions_RequiresLock = (*Compute)();
	ensure(CachedPositions_RequiresLock->Num() > 0);

	return *CachedPositions_RequiresLock;
}

const FVoxelPositionQueryParameter::FCells& FVoxelPositionQueryParameter::GetCells_RequiresLock() const
{
	if (CachedCells_RequiresLock)
	{
		return *CachedCells_RequiresLock;
	}

	VOXEL_SCOPE_COUNTER("Compute cells");

	const FVoxelBox Bounds = GetBounds_RequiresLock();
	const FVoxelVectorBuffer Positions = GetPositions_RequiresLock();
	const double MaxSize = Bounds.Size().GetMax();

	check(Positions.Num() == Positions.X.Num());
	check(Positions.Num() == Positions.Y.Num());
	check(Positions.Num() == Positions.Z.Num());

	const int32 Size = FMath::CeilToInt(FMath::Pow(double(Positions.Num()), 1. / 3.) / 4);
	const int32 Step = FMath::CeilToInt(MaxSize / Size);

	FIntVector GridSize = FVoxelUtilities::CeilToInt(Bounds.Size() / Step);
	// GridSize will be 0 if all positions are the same on any axis
	GridSize = FVoxelUtilities::ComponentMax(GridSize, FIntVector(1));

	FVoxelInt64BufferStorage CellsStorage;
	CellsStorage.Allocate(GridSize.X * GridSize.Y * GridSize.Z);
	CellsStorage.Memzero();

	ForeachVoxelBufferChunk_Sync("VoxelPositionQueryParameter_CountCells", Positions.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelPositionQueryParameter_CountCells(
			Positions.X.GetData(Iterator),
			Positions.Y.GetData(Iterator),
			Positions.Z.GetData(Iterator),
			Step,
			GetISPCValue(FVector3f(Bounds.Min)),
			GetISPCValue(GridSize),
			ReinterpretCastPtr<ispc::FCell*>(CellsStorage.GetISPCBuffer()),
			Iterator.Num());
	});

	{
		VOXEL_SCOPE_COUNTER("Assign cell indices");

		int32 Index = 0;
		for (int64& CellBytes : CellsStorage)
		{
			FCell& Cell = ReinterpretCastRef<FCell>(CellBytes);
			Cell.Index = Index;
			Index += Cell.Num;
		}
		ensure(Index == Positions.Num());
	}

	FVoxelInt32BufferStorage IndicesStorage;
	IndicesStorage.Allocate(Positions.Num());

	ForeachVoxelBufferChunk_Sync("VoxelPositionQueryParameter_WriteCells", Positions.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelPositionQueryParameter_WriteCells(
			Positions.X.GetData(Iterator),
			Positions.Y.GetData(Iterator),
			Positions.Z.GetData(Iterator),
			Step,
			GetISPCValue(FVector3f(Bounds.Min)),
			GetISPCValue(GridSize),
			ReinterpretCastPtr<ispc::FCell*>(CellsStorage.GetISPCBuffer()),
			Iterator.GetIndex(),
			IndicesStorage.GetISPCBuffer(),
			Iterator.Num());
	});

	{
		VOXEL_SCOPE_COUNTER("Fix cell indices");

		int32 Index = 0;
		for (int64& CellBytes : CellsStorage)
		{
			FCell& Cell = ReinterpretCastRef<FCell>(CellBytes);
			Cell.Index -= Cell.Num;

			checkVoxelSlow(Cell.Index == Index);
			Index += Cell.Num;
		}
	}

	CachedCells_RequiresLock = FCells();
	CachedCells_RequiresLock->Cells = FVoxelInt64Buffer::Make(CellsStorage);
	CachedCells_RequiresLock->Indices = FVoxelInt32Buffer::Make(IndicesStorage);
	CachedCells_RequiresLock->Step = Step;
	CachedCells_RequiresLock->Offset = Bounds.Min;
	CachedCells_RequiresLock->GridSize = GridSize;
	return *CachedCells_RequiresLock;
}