// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelActorBase.h"
#include "VoxelLandscape.generated.h"

UCLASS()
class VOXELLANDSCAPE_API AVoxelLandscape : public AVoxelActorBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (Units = cm))
	int32 VoxelSize = 100;

	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (UIMin = 0.1, UIMax = 2))
	double TargetVoxelSizeOnScreen = 1.f;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay, meta = (UIMin = 2, UIMax = 8))
	int32 DetailTextureSize = 2;

	UPROPERTY(EditAnywhere, Category = "Config", AdvancedDisplay)
	bool bEnableNanite = false;

	//~ Begin AVoxelActorBase Interface
	virtual TSharedPtr<IVoxelActorRuntime> CreateNewRuntime() override;
	//~ End AVoxelActorBase Interface
};