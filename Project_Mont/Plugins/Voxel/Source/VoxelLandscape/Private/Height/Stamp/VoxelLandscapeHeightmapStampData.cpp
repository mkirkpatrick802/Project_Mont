// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/Stamp/VoxelLandscapeHeightmapStampData.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelLandscapeHeightmapStampMemory);

FVoxelLandscapeHeightmapStampData::FVoxelLandscapeHeightmapStampData()
{
	SizeX = 4;
	SizeY = 4;
	Heights.SetNumZeroed(SizeX * SizeY);
}

void FVoxelLandscapeHeightmapStampData::Initialize(
	const int32 NewSizeX,
	const int32 NewSizeY,
	TVoxelArray64<uint16>&& NewHeights)
{
	SizeX = NewSizeX;
	SizeY = NewSizeY;
	Heights = MoveTemp(NewHeights);
}

void FVoxelLandscapeHeightmapStampData::Serialize(FArchive& Ar)
{
	Ar << SizeX;
	Ar << SizeY;
	Heights.BulkSerialize(Ar);
}

int64 FVoxelLandscapeHeightmapStampData::GetAllocatedSize() const
{
	return Heights.Num();
}