// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

// ReSharper disable CppCStyleCast

export void VoxelBufferConversionUtilities_IntToFloat(
	const uniform int32 Int[],
	uniform float Float[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Float[Index] = (float)Int[Index];
	}
}

export void VoxelBufferConversionUtilities_IntToDouble(
	const uniform int32 Int[],
	uniform double Double[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Double[Index] = (double)Int[Index];
	}
}

export void VoxelBufferConversionUtilities_FloatToDouble(
	const uniform float Float[],
	uniform double Double[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Double[Index] = (double)Float[Index];
	}
}

export void VoxelBufferConversionUtilities_DoubleToFloat(
	const uniform double Double[],
	uniform float Float[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Float[Index] = (float)Double[Index];
	}
}

export void VoxelBufferConversionUtilities_PointIdToSeed(
	const uniform uint64 PointId[],
	uniform uint32 Seed[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const uint64 Value = PointId[Index];
		Seed[Index] = (uint32)Value ^ (uint32)(Value >> 32);
	}
}