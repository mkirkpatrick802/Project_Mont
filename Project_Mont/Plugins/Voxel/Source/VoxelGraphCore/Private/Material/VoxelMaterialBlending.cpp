// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialBlending.h"

FVoxelMaterialBlending FVoxelMaterialBlending::FromIndex(const int32 Index)
{
	ensure(0 <= Index && Index < (1 << 12));

	FVoxelMaterialBlending Result;
	Result.MaterialA = Index;
	Result.AlphaA = 255;
	return Result;
}

int32 FVoxelMaterialBlending::GetBestIndex() const
{
	if (AlphaA >= AlphaB &&
		AlphaA >= AlphaC)
	{
		return MaterialA;
	}

	if (AlphaB >= AlphaC)
	{
		return MaterialB;
	}

	return MaterialC;
}