// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Material/VoxelMaterialDefinitionInstance.h"
#include "Material/VoxelMaterialDefinitionInterfaceToolkit.h"
#include "VoxelMaterialDefinitionInstanceToolkit.generated.h"

USTRUCT()
struct FVoxelMaterialDefinitionInstanceToolkit : public FVoxelMaterialDefinitionInterfaceToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelMaterialDefinitionInstance> Asset;

	//~ Begin FVoxelSimpleAssetToolkit Interface
	virtual TSharedPtr<FTabManager::FLayout> GetLayout() const override;
	//~ End FVoxelSimpleAssetToolkit Interface
};