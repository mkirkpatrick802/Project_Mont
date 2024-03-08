// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelSimpleAssetToolkit.h"
#include "VoxelMaterialDefinitionInterfaceToolkit.generated.h"

USTRUCT()
struct FVoxelMaterialDefinitionInterfaceToolkit : public FVoxelSimpleAssetToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	//~ Begin FVoxelSimpleAssetToolkit Interface
	virtual void PostEditChange(const FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShowFloor() const override { return false; }
	virtual void SetupPreview() override;
	virtual void UpdatePreview() override;
	virtual FRotator GetInitialViewRotation() const override;
	virtual TOptional<float> GetInitialViewDistance() const override { return 500; }
	//~ End FVoxelSimpleAssetToolkit Interface

private:
	TWeakObjectPtr<UStaticMeshComponent> StaticMeshComponent;
};