// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFunctionLibraryAssetToolkit.h"
#include "VoxelGraphToolkit.h"

TArray<FVoxelToolkit::FMode> FVoxelFunctionLibraryAssetToolkit::GetModes() const
{
	FMode Mode;
	Mode.Struct = FVoxelGraphToolkit::StaticStruct();
	Mode.Object = &Asset->GetGraph();
	return { Mode };
}