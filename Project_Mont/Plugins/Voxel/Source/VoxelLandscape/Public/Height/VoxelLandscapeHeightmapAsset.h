// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeHeightmapAsset.generated.h"

#if 0 // TODO Asset with chunk & LOD streaming
UCLASS(HideDropdown, BlueprintType, meta = (VoxelAssetType, AssetColor=Red))
class VOXELLANDSCAPE_API UVoxelLandscapeHeightmapAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Info")
	int32 Width = 0;

	UPROPERTY(VisibleAnywhere, Category = "Info")
	int32 Height = 0;

	UPROPERTY(EditAnywhere, Category = "Info", meta = (FilePathFilter = "Heightmap file (*.png; *.raw; *.r16)|*.png;*.raw;*.r16"))
	FFilePath ImportPath;

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	EVoxelLandscapeHeightmapInterpolationType Interpolation = EVoxelLandscapeHeightmapInterpolationType::Bicubic;

	UPROPERTY(EditAnywhere, Category = "Config")
	double OffsetZ = 0;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (Units = cm))
	float ScaleXY = 1;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (Units = cm))
	float ScaleZ = 1;

public:
	UPROPERTY(EditAnywhere, Category = "Min Height")
	bool bEnableMinHeight = false;

	UPROPERTY(EditAnywhere, Category = "Min Height", meta = (EditCondition = bEnableMinHeight, UIMin = 0, UIMax = 1))
	float MinHeight = 0;

	UPROPERTY(EditAnywhere, Category = "Min Height", meta = (EditCondition = bEnableMinHeight))
	float MinHeightSlope = 1;

public:
	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

private:
	FByteBulkData BulkData;
};
#endif