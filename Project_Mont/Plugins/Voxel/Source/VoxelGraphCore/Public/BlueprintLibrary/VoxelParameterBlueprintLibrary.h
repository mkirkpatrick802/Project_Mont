// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelParameterBlueprintLibrary.generated.h"

class IVoxelParameterOverridesObjectOwner;

UCLASS()
class VOXELGRAPHCORE_API UVoxelParameterBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, DisplayName = "Get Voxel Parameter", CustomThunk, Category = "Voxel|Parameters", meta = (CustomStructureParam = "Value", BlueprintInternalUseOnly = "true"))
	static void K2_GetVoxelParameter(
		TScriptInterface<IVoxelParameterOverridesObjectOwner> Owner,
		FName Name,
		int32& Value)
	{
		unimplemented();
	}
	DECLARE_FUNCTION(execK2_GetVoxelParameter);

public:
	UFUNCTION(BlueprintCallable, DisplayName = "Set Voxel Parameter", CustomThunk, Category = "Voxel|Parameters", meta = (AutoCreateRefTerm = "Value", CustomStructureParam = "Value,OutValue", BlueprintInternalUseOnly = "true"))
	static void K2_SetVoxelParameter(
		TScriptInterface<IVoxelParameterOverridesObjectOwner> Owner,
		FName Name,
		const int32& Value,
		int32& OutValue)
	{
		unimplemented();
	}
	DECLARE_FUNCTION(execK2_SetVoxelParameter);

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel|Parameters")
	static bool HasVoxelParameter(
		TScriptInterface<IVoxelParameterOverridesObjectOwner> Owner,
		FName Name);
};