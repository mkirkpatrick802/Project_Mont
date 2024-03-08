// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeHeightmapInterpolation.generated.h"

UENUM(BlueprintType)
enum class EVoxelLandscapeHeightmapInterpolation : uint8
{
	// Fastest, rounds sample position
	NearestNeighbor,
	// Bilinear interpolation
	// Smoother than nearest, but will have a visible grid pattern
	// 2.5x slower than nearest
	Bilinear,
	// Bicubic interpolation
	// Best quality, smooth results with no artifacts
	// 4x slower than nearest, 1.6x slower than bilinear
	Bicubic
};