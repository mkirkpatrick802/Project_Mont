// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Height/VoxelLandscapeHeightProvider.h"
#include "Height/VoxelLandscapeHeightmapInterpolation.h"
#include "VoxelLandscapeTextureHeightProvider.generated.h"

UENUM(BlueprintType)
enum class EVoxelLandscapeTextureChannel : uint8
{
	R,
	G,
	B,
	A
};

DECLARE_VOXEL_MEMORY_STAT(VOXELLANDSCAPE_API, STAT_VoxelLandscapeTextureHeightDataMemory, "Voxel Landscape Texture Height Data Memory");

struct FVoxelLandscapeTextureHeightData
{
	int32 SizeX = 0;
	int32 SizeY = 0;
	int32 TypeSize = 0;
	TVoxelArray<uint8> Data;
	FFloatInterval Range;

	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelLandscapeTextureHeightDataMemory);

	int64 GetAllocatedSize() const
	{
		return Data.GetAllocatedSize();
	}
};

USTRUCT()
struct VOXELLANDSCAPE_API FVoxelLandscapeTextureHeightProvider : public FVoxelLandscapeHeightProvider
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, Category = "Config")
	EVoxelLandscapeTextureChannel Channel = EVoxelLandscapeTextureChannel::R;

	UPROPERTY(EditAnywhere, Category = "Config")
	float ScaleXY = 100;

	UPROPERTY(EditAnywhere, Category = "Config")
	float ScaleZ = 100000;

	UPROPERTY(EditAnywhere, Category = "Config")
	float OffsetZ = 0;

	UPROPERTY(EditAnywhere, Category = "Config")
	EVoxelLandscapeHeightmapInterpolation Interpolation = EVoxelLandscapeHeightmapInterpolation::Bicubic;

	UPROPERTY(EditAnywhere, Category = "Config", meta = (UIMin = 0, UIMax = 1))
	float BicubicSmoothness = 1;
public:
	//~ Begin FVoxelLandscapeHeightProvider Interface
	virtual bool TryInitialize() override;
	virtual FVoxelBox2D GetBounds() const override;
	virtual FVoxelFuture Apply(const FVoxelLandscapeHeightQuery& Query) const override;
	//~ End FVoxelLandscapeHeightProvider Interface

private:
	TSharedPtr<const FVoxelLandscapeTextureHeightData> TextureData;
};

class VOXELLANDSCAPE_API FVoxelLandscapeTextureHeightCache : public FVoxelSingleton
{
public:
	//~ Begin FVoxelSingleton Interface
	virtual void Tick() override;
	//~ End FVoxelSingleton Interface

	TSharedPtr<const FVoxelLandscapeTextureHeightData> GetHeightData_GameThread(
		const UTexture2D* Texture,
		EVoxelLandscapeTextureChannel Channel);

private:
	struct FHeightDataRef
	{
		TSharedPtr<const FVoxelLandscapeTextureHeightData> HeightData;
		double LastReferencedTime = FPlatformTime::Seconds();
	};

	using FChannelToData = TVoxelInlineMap<
		EVoxelLandscapeTextureChannel,
		TWeakPtr<const FVoxelLandscapeTextureHeightData>,
		1>;

	TVoxelArray<FHeightDataRef> HeightDataRefs;
	TVoxelMap<FObjectKey, FChannelToData> TextureToChannelToData;

	static TSharedPtr<FVoxelLandscapeTextureHeightData> CreateHeightData_GameThread(
		const UTexture2D* Texture,
		EVoxelLandscapeTextureChannel Channel);
};
extern VOXELLANDSCAPE_API FVoxelLandscapeTextureHeightCache* GVoxelLandscapeTextureCache;