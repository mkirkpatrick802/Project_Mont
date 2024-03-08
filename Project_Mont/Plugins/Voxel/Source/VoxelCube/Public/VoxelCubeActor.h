// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelActorBase.h"
#include "VoxelCubeActor.generated.h"

UCLASS()
class VOXELCUBE_API AVoxelCubeActor : public AVoxelActorBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (Units = cm))
	int32 VoxelSize = 10;
};