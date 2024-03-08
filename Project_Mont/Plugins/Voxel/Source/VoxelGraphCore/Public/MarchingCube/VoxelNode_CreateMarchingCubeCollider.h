// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelPhysicalMaterial.h"
#include "MarchingCube/VoxelMarchingCubeSurface.h"
#include "MarchingCube/VoxelMarchingCubeCollider.h"
#include "VoxelNode_CreateMarchingCubeCollider.generated.h"

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelNode_CreateMarchingCubeCollider : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMarchingCubeSurface, Surface, nullptr);
	VOXEL_INPUT_PIN(FVoxelPhysicalMaterialBuffer, PhysicalMaterial, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelMarchingCubeColliderWrapper, Collider);
};