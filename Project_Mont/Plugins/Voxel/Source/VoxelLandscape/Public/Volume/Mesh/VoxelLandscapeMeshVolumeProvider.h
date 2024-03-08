// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Volume/VoxelLandscapeVolumeProvider.h"
#include "VoxelLandscapeMeshVolumeProvider.generated.h"

class FVoxelVoxelizedMeshData;

USTRUCT()
struct FVoxelLandscapeMeshVolumeProvider : public FVoxelLandscapeVolumeProvider
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UStaticMesh> Mesh;

	//~ Begin FVoxelLandscapeVolumeProvider Interface
	virtual bool TryInitialize() override;
	virtual FVoxelBox GetBounds() const override;
	virtual FVoxelFuture Apply(const FVoxelLandscapeVolumeQuery& Query) const override;
	//~ End FVoxelLandscapeVolumeProvider Interface

private:
	TSharedPtr<const FVoxelVoxelizedMeshData> MeshData;
};