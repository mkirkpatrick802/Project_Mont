// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelChannelName.generated.h"

USTRUCT(BlueprintType, DisplayName = "Voxel Channel")
struct VOXELGRAPHCORE_API FVoxelChannelName
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FName Name = "Project.Surface";
};