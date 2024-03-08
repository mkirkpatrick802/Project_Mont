// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Height/VoxelLandscapeHeightmapInterpolation.h"
#include "Height/Stamp/VoxelLandscapeHeightmapStampData.h"
#include "VoxelLandscapeHeightmapStampAsset.generated.h"

UCLASS(HideDropdown, BlueprintType, meta = (VoxelAssetType, AssetColor=Red))
class VOXELLANDSCAPE_API UVoxelLandscapeHeightmapStampAsset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (Units = cm))
	float ScaleXY = 1;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (Units = cm))
	float ScaleZ = 1;

	UPROPERTY(EditAnywhere, Category = "Config")
	float OffsetZ = 0;

	UPROPERTY(EditAnywhere, Category = "Config")
	EVoxelLandscapeHeightmapInterpolation Interpolation = EVoxelLandscapeHeightmapInterpolation::Bicubic;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (UIMin = 0, UIMax = 1))
	float BicubicSmoothness = 1;
public:
	UPROPERTY(EditAnywhere, Category = "Min Height")
	bool bEnableMinHeight = false;

	UPROPERTY(EditAnywhere, Category = "Min Height", meta = (EditCondition = bEnableMinHeight, UIMin = 0, UIMax = 1))
	float MinHeight = 0;

	UPROPERTY(EditAnywhere, Category = "Min Height", meta = (EditCondition = bEnableMinHeight))
	float MinHeightSlope = 1;

public:
	UPROPERTY(EditAnywhere, Category = "Import", meta = (FilePathFilter = "Heightmap file (*.png; *.raw; *.r16)|*.png;*.raw;*.r16"))
	FFilePath ImportPath;

public:
	void Import(const TSharedRef<FVoxelLandscapeHeightmapStampData>& NewSourceStampData);

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

public:
	// (Z * InternalScaleZ + InternalOffsetZ) * ScaleZ + OffsetZ
	float GetFinalScaleZ() const
	{
		return InternalScaleZ * ScaleZ;
	}
	float GetFinalOffsetZ() const
	{
		return InternalOffsetZ * ScaleZ + OffsetZ;
	}
	TSharedRef<const FVoxelLandscapeHeightmapStampData> GetStampData() const
	{
		return StampData;
	}

private:
	UPROPERTY()
	float InternalScaleZ = 1.f;

	UPROPERTY()
	float InternalOffsetZ = 0.f;

	FByteBulkData BulkData;
	FByteBulkData SourceBulkData;

	TSharedRef<FVoxelLandscapeHeightmapStampData> StampData = MakeVoxelShared<FVoxelLandscapeHeightmapStampData>();
	TSharedRef<FVoxelLandscapeHeightmapStampData> SourceStampData = MakeVoxelShared<FVoxelLandscapeHeightmapStampData>();

	void UpdateFromSource();
};