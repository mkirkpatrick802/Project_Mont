// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraph.h"
#include "VoxelFunctionLibraryAsset.generated.h"

UCLASS(meta = (VoxelAssetType, AssetColor = Blue))
class VOXELGRAPHCORE_API UVoxelFunctionLibraryAsset : public UObject
{
	GENERATED_BODY()

public:
	UVoxelFunctionLibraryAsset();

	UVoxelGraph& GetGraph()
	{
		return *Graph;
	}
	const UVoxelGraph& GetGraph() const
	{
		return *Graph;
	}

private:
	UPROPERTY()
	TObjectPtr<UVoxelGraph> Graph;
};