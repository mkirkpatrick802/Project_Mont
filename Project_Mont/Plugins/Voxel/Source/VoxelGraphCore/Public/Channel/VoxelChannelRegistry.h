// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"
#include "VoxelChannelRegistry.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelChannelExposedDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Config")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelPinType Type;

	UPROPERTY(EditAnywhere, Category = "Config")
	FVoxelPinValue DefaultValue;

	void Fixup();
};

UCLASS(meta = (VoxelAssetType, AssetColor=Blue))
class VOXELGRAPHCORE_API UVoxelChannelRegistry : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TArray<FVoxelChannelExposedDefinition> Channels;

	void UpdateChannels();

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void BeginDestroy() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
};