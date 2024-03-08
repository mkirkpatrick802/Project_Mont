// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValueInterface.generated.h"

USTRUCT()
struct FVoxelPinValueInterface : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	virtual void Fixup() {}
	virtual void ComputeRuntimeData() {}
	virtual void ComputeExposedData() {}
};