// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelSculptBlueprintLibrary.generated.h"

class AVoxelActor;

UCLASS()
class VOXELGRAPHCORE_API UVoxelSculptBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Sculpt")
	static void ApplySculpt(
		AVoxelActor* TargetActor,
		AVoxelActor* SculptActor);
};