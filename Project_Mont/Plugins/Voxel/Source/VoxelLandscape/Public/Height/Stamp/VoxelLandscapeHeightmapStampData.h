// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

DECLARE_VOXEL_MEMORY_STAT(VOXELLANDSCAPE_API, STAT_VoxelLandscapeHeightmapStampMemory, "Voxel Landscape Heightmap Stamp Memory");

class VOXELLANDSCAPE_API FVoxelLandscapeHeightmapStampData
{
public:
	FVoxelLandscapeHeightmapStampData();

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelLandscapeHeightmapStampMemory);

	void Initialize(
		int32 NewSizeX,
		int32 NewSizeY,
		TVoxelArray64<uint16>&& NewHeights);

	void Serialize(FArchive& Ar);
	int64 GetAllocatedSize() const;

public:
	FORCEINLINE int32 GetSizeX() const
	{
		return SizeX;
	}
	FORCEINLINE int32 GetSizeY() const
	{
		return SizeY;
	}
	FORCEINLINE TConstVoxelArrayView64<uint16> GetHeights() const
	{
		return Heights;
	}
	FORCEINLINE float GetNormalizedHeight(const int32 Index) const
	{
		return Heights[Index] / float(MAX_uint16);
	}
	FORCEINLINE float GetNormalizedHeight(const int32 X, const int32 Y) const
	{
		return GetNormalizedHeight(FVoxelUtilities::Get2DIndex<int32>(SizeX, SizeY, X, Y));
	}

private:
	int32 SizeX = 0;
	int32 SizeY = 0;
	TVoxelArray64<uint16> Heights;
};