// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"
#include "VoxelBufferStorageImpl.isph"

// ReSharper disable CppCStyleCast

struct FCell
{
	int32 Num;
	int32 Index;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelPositionQueryParameter_WritePositions3D(
	uniform float DataX[],
	uniform float DataY[],
	uniform float DataZ[],
	const uniform float OffsetX,
	const uniform float OffsetY,
	const uniform float OffsetZ,
	const uniform int32 SizeX,
	const uniform int32 SizeY,
	const uniform float Step,
	uniform int32 StartIndex,
	const uniform int32 Num)
{
	DataX -= StartIndex;
	DataY -= StartIndex;
	DataZ -= StartIndex;

	const uniform int32 SizeXY = SizeX * SizeY;
	uniform int32 EndIndex = StartIndex + Num;

	{
		const uniform int32 StartZ = StartIndex / SizeXY;
		const uniform int32 StartY = (StartIndex - StartZ * SizeXY) / SizeX;
		const uniform int32 StartX = StartIndex - StartZ * SizeXY - StartY * SizeX;

		if (StartX != 0)
		{
			const uniform float ValueY = OffsetY + StartY * Step;
			const uniform float ValueZ = OffsetZ + StartZ * Step;

			StartIndex -= StartX;

			FOREACH(X, StartX, SizeX)
			{
				DataX[StartIndex + X] = OffsetX + X * Step;
				DataY[StartIndex + X] = ValueY;
				DataZ[StartIndex + X] = ValueZ;
			}

			StartIndex += SizeX;
		}
	}

	if (StartIndex == EndIndex)
	{
		return;
	}

	{
		const uniform int32 LastIndex = EndIndex - 1;
		const uniform int32 LastZ = LastIndex / SizeXY;
		const uniform int32 LastY = (LastIndex - LastZ * SizeXY) / SizeX;
		const uniform int32 LastX = LastIndex - LastZ * SizeXY - LastY * SizeX;

		if (LastX != SizeX - 1)
		{
			const uniform float ValueY = OffsetY + LastY * Step;
			const uniform float ValueZ = OffsetZ + LastZ * Step;
			const uniform int32 BaseIndex = LastIndex - LastX;

			FOREACH(X, 0, LastX + 1)
			{
				DataX[BaseIndex + X] = OffsetX + X * Step;
				DataY[BaseIndex + X] = ValueY;
				DataZ[BaseIndex + X] = ValueZ;
			}

			EndIndex -= LastX + 1;
		}
	}

	if (StartIndex == EndIndex)
	{
		return;
	}

	check(StartIndex % SizeX == 0);
	check(EndIndex % SizeX == 0);

	uniform int32 Z = StartIndex / SizeXY;
	uniform int32 Y = (StartIndex - Z * SizeXY) / SizeX;

	for (uniform int32 Index = StartIndex; Index < EndIndex; Index += SizeX)
	{
		FOREACH(X, 0, SizeX)
		{
			DataX[Index + X] = OffsetX + X * Step;
		}

		const uniform float ValueY = OffsetY + Y * Step;
		FOREACH(X, 0, SizeX)
		{
			DataY[Index + X] = ValueY;
		}

		const uniform float ValueZ = OffsetZ + Z * Step;
		FOREACH(X, 0, SizeX)
		{
			DataZ[Index + X] = ValueZ;
		}

		Y++;

		if (Y == SizeY)
		{
			Y = 0;
			Z++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelPositionQueryParameter_CountCells(
	const uniform float PositionsX[],
	const uniform float PositionsY[],
	const uniform float PositionsZ[],
	const uniform float Step,
	const uniform float3& Offset,
	const uniform int3& GridSize,
	DeclareBuffer(FCell, Cells),
	const uniform int32 Num)
{
	const uniform int32 NumBlocks = DivideFloor(Num, 8);

	for (uniform int32 BlockIndex = 0; BlockIndex < NumBlocks; BlockIndex++)
	{
		uniform int32 GridIndices[8];

		FOREACH(Wave, 0, 8)
		{
			const varying int32 Index = 8 * BlockIndex + Wave;

			varying float PositionX = PositionsX[Index] - Offset.x;
			varying float PositionY = PositionsY[Index] - Offset.y;
			varying float PositionZ = PositionsZ[Index] - Offset.z;

			PositionX /= Step;
			PositionY /= Step;
			PositionZ /= Step;

			varying int32 IndexX = floor(PositionX);
			varying int32 IndexY = floor(PositionY);
			varying int32 IndexZ = floor(PositionZ);

			IndexX = clamp(IndexX, 0, GridSize.x - 1);
			IndexY = clamp(IndexY, 0, GridSize.y - 1);
			IndexZ = clamp(IndexZ, 0, GridSize.z - 1);

			GridIndices[Wave] =
				IndexX +
				IndexY * GridSize.x +
				IndexZ * GridSize.x * GridSize.y;
		}

		UNROLL
		for (uniform int32 Wave = 0; Wave < 8; Wave++)
		{
			const uniform int32 GridIndex = GridIndices[Wave];
			GetBuffer_NotConstant(Cells, GridIndex).Num++;
		}
	}

	for (uniform int32 Index = 8 * NumBlocks; Index < Num; Index++)
	{
		uniform float PositionX = PositionsX[Index] - Offset.x;
		uniform float PositionY = PositionsY[Index] - Offset.y;
		uniform float PositionZ = PositionsZ[Index] - Offset.z;

		PositionX /= Step;
		PositionY /= Step;
		PositionZ /= Step;

		uniform int32 IndexX = floor(PositionX);
		uniform int32 IndexY = floor(PositionY);
		uniform int32 IndexZ = floor(PositionZ);

		IndexX = clamp(IndexX, 0, GridSize.x - 1);
		IndexY = clamp(IndexY, 0, GridSize.y - 1);
		IndexZ = clamp(IndexZ, 0, GridSize.z - 1);

		const uniform int32 GridIndex =
			IndexX +
			IndexY * GridSize.x +
			IndexZ * GridSize.x * GridSize.y;

		GetBuffer_NotConstant(Cells, GridIndex).Num++;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelPositionQueryParameter_WriteCells(
	const uniform float PositionsX[],
	const uniform float PositionsY[],
	const uniform float PositionsZ[],
	const uniform float Step,
	const uniform float3& Offset,
	const uniform int3& GridSize,
	DeclareBuffer(FCell, Cells),
	const uniform int32 IndexOffset,
	DeclareBuffer(int32, Indices),
	const uniform int32 Num)
{
	const uniform int32 NumBlocks = DivideFloor(Num, 8);

	for (uniform int32 BlockIndex = 0; BlockIndex < NumBlocks; BlockIndex++)
	{
		uniform int32 GridIndices[8];

		FOREACH(Wave, 0, 8)
		{
			const varying int32 Index = 8 * BlockIndex + Wave;

			varying float PositionX = PositionsX[Index] - Offset.x;
			varying float PositionY = PositionsY[Index] - Offset.y;
			varying float PositionZ = PositionsZ[Index] - Offset.z;

			PositionX /= Step;
			PositionY /= Step;
			PositionZ /= Step;

			varying int32 IndexX = floor(PositionX);
			varying int32 IndexY = floor(PositionY);
			varying int32 IndexZ = floor(PositionZ);

			IndexX = clamp(IndexX, 0, GridSize.x - 1);
			IndexY = clamp(IndexY, 0, GridSize.y - 1);
			IndexZ = clamp(IndexZ, 0, GridSize.z - 1);

			GridIndices[Wave] =
				IndexX +
				IndexY * GridSize.x +
				IndexZ * GridSize.x * GridSize.y;
		}

		UNROLL
		for (uniform int32 Wave = 0; Wave < 8; Wave++)
		{
			const uniform int32 GridIndex = GridIndices[Wave];
			uniform FCell& Cell = GetBuffer_NotConstant(Cells, GridIndex);

			const uniform int32 WriteIndex = Cell.Index;
			Cell.Index++;

			GetBuffer_NotConstant(Indices, WriteIndex) = IndexOffset + 8 * BlockIndex + Wave;
		}
	}

	for (uniform int32 Index = 8 * NumBlocks; Index < Num; Index++)
	{
		uniform float PositionX = PositionsX[Index] - Offset.x;
		uniform float PositionY = PositionsY[Index] - Offset.y;
		uniform float PositionZ = PositionsZ[Index] - Offset.z;

		PositionX /= Step;
		PositionY /= Step;
		PositionZ /= Step;

		uniform int32 IndexX = floor(PositionX);
		uniform int32 IndexY = floor(PositionY);
		uniform int32 IndexZ = floor(PositionZ);

		IndexX = clamp(IndexX, 0, GridSize.x - 1);
		IndexY = clamp(IndexY, 0, GridSize.y - 1);
		IndexZ = clamp(IndexZ, 0, GridSize.z - 1);

		check(0 <= IndexX && IndexX < GridSize.x);
		check(0 <= IndexY && IndexY < GridSize.y);
		check(0 <= IndexZ && IndexZ < GridSize.z);

		const uniform int32 GridIndex =
			IndexX +
			IndexY * GridSize.x +
			IndexZ * GridSize.x * GridSize.y;

		uniform FCell& Cell = GetBuffer_NotConstant(Cells, GridIndex);

		const uniform int32 WriteIndex = Cell.Index;
		Cell.Index++;

		GetBuffer_NotConstant(Indices, WriteIndex) = IndexOffset + Index;
	}
}