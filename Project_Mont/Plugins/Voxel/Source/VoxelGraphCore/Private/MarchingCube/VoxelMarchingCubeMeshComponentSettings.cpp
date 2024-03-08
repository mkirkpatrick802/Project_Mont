// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelMarchingCubeMeshComponentSettings.h"
#include "MarchingCube/VoxelMarchingCubeMeshComponent.h"

void FVoxelMarchingCubeMeshComponentSettings::ApplyToComponent(UVoxelMarchingCubeMeshComponent& Component) const
{
	Component.BoundsExtension = BoundsExtension;

	Super::ApplyToComponent(Component);
}