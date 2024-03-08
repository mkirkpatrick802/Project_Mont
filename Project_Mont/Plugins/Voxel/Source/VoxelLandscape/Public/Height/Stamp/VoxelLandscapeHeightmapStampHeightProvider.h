// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Height/VoxelLandscapeHeightProvider.h"
#include "Height/VoxelLandscapeHeightmapInterpolation.h"
#include "VoxelLandscapeHeightmapStampHeightProvider.generated.h"

class FVoxelLandscapeHeightmapStampData;
class UVoxelLandscapeHeightmapStampAsset;

USTRUCT()
struct VOXELLANDSCAPE_API FVoxelLandscapeHeightmapStampHeightProvider : public FVoxelLandscapeHeightProvider
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelLandscapeHeightmapStampAsset> Stamp;

public:
	//~ Begin FVoxelLandscapeHeightProvider Interface
	virtual bool TryInitialize() override;
	virtual FVoxelBox2D GetBounds() const override;
	virtual FVoxelFuture Apply(const FVoxelLandscapeHeightQuery& Query) const override;
	//~ End FVoxelLandscapeHeightProvider Interface

private:
	struct FStampProxy
	{
		const EVoxelLandscapeHeightmapInterpolation Interpolation;
		const float ScaleXY;
		const float ScaleZ;
		const float OffsetZ;
		const float BicubicSmoothness;
		const TSharedRef<const FVoxelLandscapeHeightmapStampData> StampData;

		explicit FStampProxy(const UVoxelLandscapeHeightmapStampAsset& Asset);
	};
	TSharedPtr<FStampProxy> Proxy;
};