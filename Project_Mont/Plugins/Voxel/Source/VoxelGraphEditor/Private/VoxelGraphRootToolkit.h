// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "VoxelSimpleAssetToolkit.h"
#include "VoxelGraphRootToolkit.generated.h"

class AVoxelActor;

USTRUCT()
struct FVoxelGraphRootToolkit : public FVoxelToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelGraph> Asset;

public:
	//~ Begin FVoxelToolkit Interface
	virtual TArray<FMode> GetModes() const override;
	virtual UScriptStruct* GetDefaultMode() const override;
	//~ End FVoxelToolkit Interface
};

USTRUCT(meta = (Internal))
struct FVoxelGraphPreviewToolkit : public FVoxelSimpleAssetToolkit
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	UPROPERTY()
	TObjectPtr<UVoxelGraph> Asset;

public:
	//~ Begin FVoxelSimpleAssetToolkit Interface
	virtual bool ShowFloor() const override { return false; }
	virtual void SetupPreview() override;
	virtual TOptional<float> GetInitialViewDistance() const override;
	//~ End FVoxelSimpleAssetToolkit Interface

private:
	UPROPERTY()
	TObjectPtr<AVoxelActor> VoxelActor;
};