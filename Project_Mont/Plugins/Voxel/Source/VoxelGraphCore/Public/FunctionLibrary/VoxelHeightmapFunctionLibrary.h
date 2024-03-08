// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFunctionLibrary.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Heightmap/VoxelHeightmap.h"
#include "VoxelHeightmapFunctionLibrary.generated.h"

UCLASS()
class VOXELGRAPHCORE_API UVoxelHeightmapFunctionLibrary : public UVoxelFunctionLibrary
{
	GENERATED_BODY()

public:
	// Will clamp position if outside of the heightmap bounds
	// Heightmap is centered, ie position is between -Size/2 and Size/2
	// @param	BicubicSmoothness	Smoothness of the bicubic interpolation, between 0 and 1
	UFUNCTION(Category = "Heightmap", meta = (AdvancedDisplay = "Interpolation, BicubicSmoothness"))
	FVoxelFloatBuffer SampleHeightmap(
		const FVoxelHeightmapRef& Heightmap,
		const FVoxelVector2DBuffer& Position,
		EVoxelHeightmapInterpolationType Interpolation = EVoxelHeightmapInterpolationType::Bicubic,
		float BicubicSmoothness = 1.f) const;

	UFUNCTION(Category = "Heightmap")
	FVoxelBox GetHeightmapBounds(const FVoxelHeightmapRef& Heightmap) const;
};