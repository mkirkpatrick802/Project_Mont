// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFunctionLibraryAsset.h"

DEFINE_VOXEL_FACTORY(UVoxelFunctionLibraryAsset);

UVoxelFunctionLibraryAsset::UVoxelFunctionLibraryAsset()
{
	Graph = CreateDefaultSubobject<UVoxelGraph>("Graph");
}