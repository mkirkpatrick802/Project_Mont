// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeHeightBlendMode.generated.h"

UENUM(BlueprintType)
enum class EVoxelLandscapeHeightBlendMode : uint8
{
	Max,
	Min,
	Add,
	Subtract,
	Override,
};