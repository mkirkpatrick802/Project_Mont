// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameter.h"
#include "VoxelMaterialDefinitionParameter.h"
#include "VoxelMaterialDefinitionInterface.h"
#include "VoxelMaterialDefinition.generated.h"

struct FVoxelMaterialParameterData;
class UVoxelMaterialDefinitionInstance;

UCLASS(meta = (VoxelAssetType))
class VOXELGRAPHCORE_API UVoxelMaterialDefinition : public UVoxelMaterialDefinitionInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UMaterialInterface> Material;

	UPROPERTY()
	TMap<FGuid, FVoxelMaterialDefinitionParameter> GuidToMaterialParameter;

	UPROPERTY()
#if CPP
	TMap<FGuid, TVoxelInstancedStruct<FVoxelMaterialParameterData>> GuidToParameterData;
#else
	TMap<FGuid, FVoxelInstancedStruct> GuidToParameterData;
#endif

	UPROPERTY()
	TArray<FVoxelParameter> Parameters_DEPRECATED;

	UPROPERTY()
	TMap<FGuid, FVoxelParameter> GuidToParameter_DEPRECATED;

	mutable FSimpleMulticastDelegate OnParametersChanged;

public:
	//~ Begin UObject Interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface

	//~ Begin UVoxelMaterialDefinitionInterface Interface
	virtual UVoxelMaterialDefinition* GetDefinition() const override
	{
		return ConstCast(this);
	}
	virtual FVoxelPinValue GetParameterValue(const FGuid& Guid) const override;
	//~ End UVoxelMaterialDefinitionInterface Interface

public:
	void Fixup();
	void QueueRebuildTextures();
	void RebuildTextures();
};