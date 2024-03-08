// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelCubePalette.generated.h"

USTRUCT(BlueprintType)
struct FVoxelCubeDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Config)
	FLinearColor Color = FLinearColor::White;

	// TODO Roughness, durability etc
};

// TODO Move to settings
// 4 bytes per cube: 3 bytes color, 1 byte type referencing project settings definition
// Magica assets store 1 byte volume + color palette + index to type mapping
UCLASS(BlueprintType, meta = (VoxelAssetType, AssetColor=Red))
class VOXELCUBE_API UVoxelCubePalette : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FVoxelCubeDefinition> Definitions;
};