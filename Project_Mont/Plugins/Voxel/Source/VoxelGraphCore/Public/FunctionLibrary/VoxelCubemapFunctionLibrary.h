// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFunctionLibrary.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "Heightmap/VoxelHeightmap.h"
#include "VoxelCubemapFunctionLibrary.generated.h"

UCLASS()
class VOXELGRAPHCORE_API UVoxelCubemapFunctionLibrary : public UVoxelFunctionLibrary
{
	GENERATED_BODY()

public:
	// Creates a planet from 6 heightmaps
	// All the heightmaps should have the same size
	// All the heightmap settings will be ignored - heightmap values will be between 0 and MaxHeight
	// @param	BicubicSmoothness	Smoothness of the bicubic interpolation, between 0 and 1
	UFUNCTION(Category = "Heightmap", meta = (Keywords = "make", AdvancedDisplay = "Interpolation, BicubicSmoothness"))
	UPARAM(DisplayName = "Distance") FVoxelFloatBuffer CreateCubemapPlanetDistanceField(
		FVoxelBox& Bounds,
		UPARAM(DisplayName = "+X") const FVoxelHeightmapRef& PosX,
		UPARAM(DisplayName = "-X") const FVoxelHeightmapRef& NegX,
		UPARAM(DisplayName = "+Y") const FVoxelHeightmapRef& PosY,
		UPARAM(DisplayName = "-Y") const FVoxelHeightmapRef& NegY,
		UPARAM(DisplayName = "+Z") const FVoxelHeightmapRef& PosZ,
		UPARAM(DisplayName = "-Z") const FVoxelHeightmapRef& NegZ,
		const FVector& PlanetCenter,
		float PlanetRadius = 100000.f,
		float MaxHeight = 10000.f,
		EVoxelHeightmapInterpolationType Interpolation = EVoxelHeightmapInterpolationType::Bicubic,
		float BicubicSmoothness = 1.f) const;
};