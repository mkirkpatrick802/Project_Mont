// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Height/VoxelLandscapeHeightProvider.h"
#include "VoxelLandscapeConstantHeightProvider.generated.h"

USTRUCT()
struct VOXELLANDSCAPE_API FVoxelLandscapeConstantHeightProvider : public FVoxelLandscapeHeightProvider
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	float Height = 0;

	UPROPERTY(EditAnywhere, Category = "Config")
	float SizeXY = 1000.f;

	//~ Begin FVoxelLandscapeHeightProvider Interface
	virtual bool TryInitialize() override;
	virtual FVoxelBox2D GetBounds() const override;
	virtual FVoxelFuture Apply(const FVoxelLandscapeHeightQuery& Query) const override;
	//~ End FVoxelLandscapeHeightProvider Interface
};