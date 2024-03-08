// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinType.h"
#include "VoxelPinValue.h"
#include "VoxelParameter.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelParameter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVoxelPinType Type;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FString Category;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FString Description;

	UPROPERTY()
	TMap<FName, FString> MetaData;
#endif

	UPROPERTY()
	FGuid DeprecatedGuid;

	UPROPERTY()
	FVoxelPinValue DeprecatedDefaultValue;

	void Fixup();
};