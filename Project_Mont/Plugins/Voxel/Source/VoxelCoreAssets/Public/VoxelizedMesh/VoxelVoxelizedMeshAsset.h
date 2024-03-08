// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshSettings.h"
#include "VoxelVoxelizedMeshAsset.generated.h"

class FVoxelVoxelizedMeshData;
class UVoxelVoxelizedMeshAsset;

UCLASS(BlueprintType, meta = (VoxelAssetType, AssetColor=Red))
class VOXELCOREASSETS_API UVoxelVoxelizedMeshAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel", AssetRegistrySearchable)
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ClampMin = 0.00001))
	int32 VoxelSize = 20;

	// Relative to the size
	// Bigger = higher memory usage but more accurate when using smooth min
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (UIMin = 0, UIMax = 1))
	float MaxSmoothness = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ShowOnlyInnerProperties))
	FVoxelVoxelizedMeshSettings VoxelizerSettings;

public:
	FSimpleMulticastDelegate OnDataChanged;

	void OnReimport();
	TSharedPtr<const FVoxelVoxelizedMeshData> GetData();

public:
	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

private:
	TSharedPtr<const FVoxelVoxelizedMeshData> PrivateData;

	TSharedPtr<FVoxelVoxelizedMeshData> CreateMeshData() const;
};