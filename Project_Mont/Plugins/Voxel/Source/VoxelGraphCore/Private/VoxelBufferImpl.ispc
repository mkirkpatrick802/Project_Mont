// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

export uniform int32 VoxelBufferStorage_CountBits(
	const uniform bool Values[],
	const uniform int32 Num)
{
	uniform int32 Result = 0;
	FOREACH(Index, 0, Num)
	{
		Result += popcnt(Values[Index]);
	}
	return Result;
}