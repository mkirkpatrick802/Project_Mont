// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelDeveloperSettings.h"
#include "Channel/VoxelChannelRegistry.h"
#include "VoxelSettings.generated.h"

class UVoxelFloatDetailTexture;

UCLASS(config = Engine, DefaultConfig, meta = (DisplayName = "Voxel Plugin"))
class VOXELGRAPHCORE_API UVoxelSettings : public UVoxelDeveloperSettings
{
	GENERATED_BODY()

public:
	UVoxelSettings()
	{
		SectionName = "Voxel Plugin";
	}

public:
	UPROPERTY(Config, EditAnywhere, Category = "Config")
	TArray<FVoxelChannelExposedDefinition> GlobalChannels;

public:
	// If true, will load UVoxelChannelRegistry on startup instead of blocking the game thread
	// If true, you need to wait until FVoxelRuntime::CanBeCreated returns true before loading any Voxel Actor
	UPROPERTY(Config, EditAnywhere, Category = "Cooked")
	bool bAsyncLoadChannelRegistries = false;

public:
	// If true graphs will have their own thumbnails rendered
	UPROPERTY(Config, EditAnywhere, Category = "Editor")
	bool bEnableGraphThumbnails = true;

public:
	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TSoftObjectPtr<UMaterialInterface> MarchingCubeDebugMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftClassPath("/Voxel/Debug/MarchingCubeDebugMaterial.MarchingCubeDebugMaterial"));

	UPROPERTY(Config, EditAnywhere, Category = "Debug")
	TArray<TSoftObjectPtr<UVoxelFloatDetailTexture>> MarchingCubeDebugDetailTextures =
	{
		TSoftObjectPtr<UVoxelFloatDetailTexture>(FSoftClassPath("/Voxel/Debug/MarchingCubeDebugDetailTextureR.MarchingCubeDebugDetailTextureR")),
		TSoftObjectPtr<UVoxelFloatDetailTexture>(FSoftClassPath("/Voxel/Debug/MarchingCubeDebugDetailTextureG.MarchingCubeDebugDetailTextureG")),
		TSoftObjectPtr<UVoxelFloatDetailTexture>(FSoftClassPath("/Voxel/Debug/MarchingCubeDebugDetailTextureB.MarchingCubeDebugDetailTextureB"))
	};

public:
	UPROPERTY(Config, EditAnywhere, Category = "Safety", meta = (ClampMin = 1))
	bool bEnablePerformanceMonitoring = true;

	// Number of frames to collect the average frame rate from
	UPROPERTY(Config, EditAnywhere, Category = "Safety", meta = (ClampMin = 2))
	int32 FramesToAverage = 128;

	// Minimum average FPS allowed in specified number of frames
	UPROPERTY(Config, EditAnywhere, Category = "Safety", meta = (ClampMin = 1))
	int32 MinFPS = 8;

	void UpdateChannels();

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
};