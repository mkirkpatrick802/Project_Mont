// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelSimpleAssetToolkit.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAsset.h"
#include "VoxelVoxelizedMeshAssetToolkit.generated.h"

class AVoxelActor;

USTRUCT()
struct FVoxelVoxelizedMeshAssetToolkit : public FVoxelSimpleAssetToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelVoxelizedMeshAsset> Asset;

public:
	//~ Begin FVoxelSimpleAssetToolkit Interface
	virtual void Tick() override;
	virtual bool ShowFloor() const override { return false; }
	virtual void SetupPreview() override;
	virtual void DrawPreview(const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual TOptional<float> GetInitialViewDistance() const override;
	//~ End FVoxelSimpleAssetToolkit Interface

private:
	UPROPERTY()
	TObjectPtr<AVoxelActor> Actor;
};