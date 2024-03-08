// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelParameterContainer_DEPRECATED.generated.h"

UCLASS()
class VOXELGRAPHCORE_API UVoxelParameterContainer_DEPRECATED : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bAlwaysEnabled = false;

	UPROPERTY()
	TObjectPtr<UObject> Provider;

	UPROPERTY()
	TMap<FVoxelParameterPath, FVoxelParameterValueOverride> ValueOverrides;

	virtual void PostInitProperties() override;

	void MigrateTo(FVoxelParameterOverrides& ParameterOverrides);
};