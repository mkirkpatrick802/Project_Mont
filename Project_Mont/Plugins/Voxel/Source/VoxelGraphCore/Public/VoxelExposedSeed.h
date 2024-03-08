// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExposedSeed.generated.h"

USTRUCT(BlueprintType, DisplayName = "Seed", meta = (TypeCategory = "Default"))
struct VOXELGRAPHCORE_API FVoxelExposedSeed
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config")
	FString Seed;

	void Randomize(const int32 RandSeed = FMath::Rand())
	{
		const FRandomStream Stream(RandSeed);

		Seed.Reset(8);
		for (int32 Index = 0; Index < 8; Index++)
		{
			Seed += TCHAR(Stream.RandRange(TEXT('A'), TEXT('Z')));
		}
	}
};