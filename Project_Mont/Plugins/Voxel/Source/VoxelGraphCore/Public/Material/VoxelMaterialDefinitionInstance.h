// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelMaterialDefinitionInterface.h"
#include "VoxelParameterOverrideCollection_DEPRECATED.h"
#include "VoxelMaterialDefinitionInstance.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMaterialParameterValueOverride
{
	GENERATED_BODY()

	UPROPERTY()
	bool bEnable = false;

	UPROPERTY()
	FVoxelPinValue Value;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FName CachedName;

	UPROPERTY()
	FString CachedCategory;
#endif
};

UCLASS(meta = (VoxelAssetType))
class VOXELGRAPHCORE_API UVoxelMaterialDefinitionInstance : public UVoxelMaterialDefinitionInterface
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FGuid, FVoxelMaterialParameterValueOverride> GuidToValueOverride;

	UPROPERTY()
	FVoxelParameterOverrides ParameterOverrides_DEPRECATED;

	// Removed in 340
	UPROPERTY()
	FVoxelParameterOverrideCollection_DEPRECATED ParameterCollection_DEPRECATED;

	// Removed in 341
	UPROPERTY()
	TObjectPtr<UVoxelParameterContainer_DEPRECATED> ParameterContainer_DEPRECATED;

public:
	UVoxelMaterialDefinitionInterface* GetParent() const
	{
		return Parent;
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetParent(UVoxelMaterialDefinitionInterface* NewParent);

private:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelMaterialDefinitionInterface> Parent;

	friend class FUVoxelMaterialDefinitionInstanceCustomization;

public:
	void Fixup();

	//~ Begin UVoxelMaterialDefinitionInterface Interface
	virtual UVoxelMaterialDefinition* GetDefinition() const override;
	virtual FVoxelPinValue GetParameterValue(const FGuid& Guid) const override;
	//~ End UVoxelMaterialDefinitionInterface Interface

	//~ Begin UObject Interface
	virtual void PostLoad() override;
	virtual void PostCDOContruct() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface
};