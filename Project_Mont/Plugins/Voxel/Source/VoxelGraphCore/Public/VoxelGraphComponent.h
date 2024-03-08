// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelGraphComponent.generated.h"

UCLASS(BlueprintType)
class VOXELGRAPHCORE_API UVoxelGraphComponent
	: public UActorComponent
	, public IVoxelParameterOverridesObjectOwner
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelGraph> Graph;

	UPROPERTY()
	FVoxelParameterOverrides ParameterOverrides;

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	virtual UVoxelGraph* GetGraph() const override
	{
		return Graph;
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetGraph(UVoxelGraph* NewGraph);

public:
	//~ Begin UActorComponent Interface
	virtual void PostEditImport() override;
	virtual void PostInitProperties() override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UActorComponent Interface

	//~ Begin IVoxelParameterOverridesOwner Interface
	virtual bool ShouldForceEnableOverride(const FVoxelParameterPath& Path) const override;
	virtual FVoxelParameterOverrides& GetParameterOverrides() override;
	//~ End IVoxelParameterOverridesOwner Interface
};