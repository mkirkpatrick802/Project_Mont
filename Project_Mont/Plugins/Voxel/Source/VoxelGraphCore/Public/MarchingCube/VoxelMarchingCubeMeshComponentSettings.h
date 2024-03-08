// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPrimitiveComponentSettings.h"
#include "VoxelMarchingCubeMeshComponentSettings.generated.h"

class UVoxelMarchingCubeMeshComponent;

USTRUCT(BlueprintType)
struct VOXELGRAPHCORE_API FVoxelMarchingCubeMeshComponentSettings : public FVoxelPrimitiveComponentSettings
{
	GENERATED_BODY()

	// Extends the bounds of the object.
	// This is useful when using World Position Offset to animate the vertices of the object outside of its bounds.
	// Warning: Increasing the bounds of an object will reduce performance and shadow quality!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	FVector BoundsExtension = FVector::ZeroVector;

	FVoxelMarchingCubeMeshComponentSettings()
	{
		bAffectDistanceFieldLighting = true;
		bCastFarShadow = true;
	}

	void ApplyToComponent(UVoxelMarchingCubeMeshComponent& Component) const;
};