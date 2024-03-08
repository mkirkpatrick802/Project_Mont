// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Engine/AssetUserData.h"
#include "VoxelVoxelizedMeshAssetUserData.generated.h"

class UVoxelVoxelizedMeshAsset;

UCLASS()
class VOXELCOREASSETS_API UVoxelVoxelizedMeshAssetUserData : public UAssetUserData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel")
	TSoftObjectPtr<UVoxelVoxelizedMeshAsset> Asset;

	static UVoxelVoxelizedMeshAsset* GetAsset(UStaticMesh& Mesh);
};