// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "BlueprintLibrary/VoxelBasicBlueprintLibrary.h"
#include "VoxelThreadPool.h"
#include "VoxelTaskExecutor.h"

int32 UVoxelBasicBlueprintLibrary::GetNumberOfVoxelTasks()
{
	return GVoxelTaskExecutor->NumTasks();
}

int32 UVoxelBasicBlueprintLibrary::GetNumberOfCpuCores()
{
	return FPlatformMisc::NumberOfCores();
}

double UVoxelBasicBlueprintLibrary::GetPhysicalMemoryInGB()
{
	return FPlatformMemory::GetStats().TotalPhysical / double(1 << 30);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TMulticastScriptDelegate<> GOnVoxelTasksBeginProcessing;
TMulticastScriptDelegate<> GOnVoxelTasksEndProcessing;

VOXEL_RUN_ON_STARTUP_GAME(RegisterBasicBlueprintLibraryDelegates)
{
	GVoxelTaskExecutor->OnBeginProcessing.AddLambda([]
	{
		GOnVoxelTasksBeginProcessing.ProcessMulticastDelegate<UObject>(nullptr);
	});
	GVoxelTaskExecutor->OnEndProcessing.AddLambda([]
	{
		GOnVoxelTasksEndProcessing.ProcessMulticastDelegate<UObject>(nullptr);
	});
}

void UVoxelBasicBlueprintLibrary::BindOnVoxelTasksBeginProcessing(const FOnVoxelTasksBeginProcessing& Delegate)
{
	GOnVoxelTasksBeginProcessing.Add(Delegate);
}

void UVoxelBasicBlueprintLibrary::BindOnVoxelTasksEndProcessing(const FOnVoxelTasksEndProcessing& Delegate)
{
	GOnVoxelTasksEndProcessing.Add(Delegate);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 UVoxelBasicBlueprintLibrary::GetNumberOfVoxelThreads()
{
	return GVoxelNumThreads;
}

void UVoxelBasicBlueprintLibrary::SetNumberOfVoxelThreads(const int32 Count)
{
	GVoxelNumThreads = Count;
}