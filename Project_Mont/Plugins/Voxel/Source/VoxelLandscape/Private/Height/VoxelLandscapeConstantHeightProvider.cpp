// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeConstantHeightProvider.h"

#include "Height/VoxelLandscapeHeightmapInterpolation.h"
#include "Height/VoxelLandscapeHeightUtilities.h"

bool FVoxelLandscapeConstantHeightProvider::TryInitialize()
{
	return true;
}

FVoxelBox2D FVoxelLandscapeConstantHeightProvider::GetBounds() const
{
	return FVoxelBox2D(-SizeXY / 2, SizeXY / 2);
}

FVoxelFuture FVoxelLandscapeConstantHeightProvider::Apply(const FVoxelLandscapeHeightQuery& Query) const
{
	TVoxelStaticArray<float, 16 * 16> Data(NoInit);
	FVoxelUtilities::SetAll(Data, Height);

	FVoxelLandscapeHeightUtilities::ApplyHeightmap(FVoxelLandscapeHeightUtilities::TArgs<float>
	{
		Query,
		SizeXY,
		1.f,
		0.f,
		0.f,
		EVoxelLandscapeHeightmapInterpolation::NearestNeighbor,
		16,
		16,
		Data
	});

	return FVoxelFuture::Done();
}