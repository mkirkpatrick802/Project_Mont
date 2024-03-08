// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelBasicBlueprintLibrary.generated.h"

DECLARE_DYNAMIC_DELEGATE(FOnVoxelTasksBeginProcessing);
DECLARE_DYNAMIC_DELEGATE(FOnVoxelTasksEndProcessing);

UCLASS()
class VOXELGRAPHCORE_API UVoxelBasicBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Returns the number of active & pending voxel tasks
	// Can be used to wait for loading to be done
	// Might be 0 the first few frames of the game, until the first tasks are queued
	UFUNCTION(BlueprintPure, Category = "Voxel")
	static int32 GetNumberOfVoxelTasks();
	
	UFUNCTION(BlueprintPure, Category = "Voxel")
	static int32 GetNumberOfCpuCores();
	
	UFUNCTION(BlueprintPure, Category = "Voxel")
	static double GetPhysicalMemoryInGB();

public:
	// Delegate will be fired when the number of voxel tasks goes from 0 to something else
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	static void BindOnVoxelTasksBeginProcessing(const FOnVoxelTasksBeginProcessing& Delegate);

	// Delegate will be fired when the number of voxel tasks goes to 0
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	static void BindOnVoxelTasksEndProcessing(const FOnVoxelTasksEndProcessing& Delegate);

public:
	UFUNCTION(BlueprintPure, Category = "Voxel")
	static int32 GetNumberOfVoxelThreads();

	// Sets the number of voxel threads
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	static void SetNumberOfVoxelThreads(int32 Count);
};