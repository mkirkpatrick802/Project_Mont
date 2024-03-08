// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"
#include "VoxelMaterialDefinitionParameter.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMaterialDefinitionParameter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVoxelPinType Type;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FVoxelPinValue DefaultValue;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Parameter")
	FString Category;

	UPROPERTY(EditAnywhere, Category = "Parameter")
	FString Description;

	UPROPERTY()
	TMap<FName, FString> MetaData;
#endif

	void Fixup();
};