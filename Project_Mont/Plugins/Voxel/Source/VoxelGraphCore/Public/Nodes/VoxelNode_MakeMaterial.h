// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Material/VoxelMaterial.h"
#include "VoxelNode_MakeMaterial.generated.h"

USTRUCT(Category = "Material")
struct VOXELGRAPHCORE_API FVoxelNode_MakeMaterial : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelMaterial, Material, nullptr);
	VOXEL_VARIADIC_INPUT_PIN(FVoxelMaterialParameter, Parameter, nullptr, 1);
	VOXEL_OUTPUT_PIN(FVoxelMaterial, OutMaterial);
};