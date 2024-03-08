// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFunctionLibrary.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelDistanceFieldFunctionLibrary.generated.h"

UCLASS()
class VOXELGRAPHCORE_API UVoxelDistanceFieldFunctionLibrary : public UVoxelFunctionLibrary
{
	GENERATED_BODY()

public:
	// Make a sphere distance field
	UFUNCTION(Category = "Distance Field", meta = (Keywords = "make"))
	UPARAM(DisplayName = "Distance") FVoxelFloatBuffer CreateSphereDistanceField(
		FVoxelBox& Bounds,
		const FVector& Center,
		float Radius = 500.f) const;

	// Make a box distance field
	// @param	Smoothness		Will smooth out the box edges, same unit as Extent
	UFUNCTION(Category = "Distance Field", meta = (Keywords = "make"))
	UPARAM(DisplayName = "Distance") FVoxelFloatBuffer CreateBoxDistanceField(
		FVoxelBox& Bounds,
		const FVector& Center,
		const FVector& Extent = FVector(500.f),
		float Smoothness = 100.f) const;
};