// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntimeProvider.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelParameterOverrideCollection_DEPRECATED.h"
#include "VoxelComponent.generated.h"

class UVoxelGraph;
class UVoxelParameterContainer_DEPRECATED;
class FVoxelRuntime;

UCLASS(ClassGroup = Voxel, DisplayName = "Voxel Component", HideCategories = ("Rendering", "Physics", "LOD", "Activation", "Collision", "Cooking", "AssetUserData"), meta = (BlueprintSpawnableComponent))
class VOXELGRAPHCORE_API UVoxelComponent
	: public USceneComponent
	, public IVoxelRuntimeProvider
	, public IVoxelParameterOverridesObjectOwner
{
	GENERATED_BODY()

public:
	FSimpleMulticastDelegate OnRuntimeCreated;
	FSimpleMulticastDelegate OnRuntimeDestroyed;

private:
	UPROPERTY(EditAnywhere, Category = "Voxel", DisplayName = "Graph")
#if CPP
	union
	{
		TObjectPtr<UVoxelGraph> Graph = nullptr;
		TObjectPtr<UVoxelGraph> Graph_NewProperty;
	};
#else
	TObjectPtr<UVoxelGraph> Graph_NewProperty;
#endif

	UPROPERTY()
	FVoxelParameterOverrides ParameterOverrides;

	friend class FUVoxelComponentCustomization;

public:
	// Removed in 340
	UPROPERTY()
	TSoftObjectPtr<UVoxelGraph> Graph_DEPRECATED;

	// Removed in 340
	UPROPERTY()
	FVoxelParameterOverrideCollection_DEPRECATED ParameterCollection_DEPRECATED;

	// Removed in 341
	UPROPERTY()
	TObjectPtr<UVoxelParameterContainer_DEPRECATED> ParameterContainer_DEPRECATED;

public:
	UVoxelComponent();
	virtual ~UVoxelComponent() override;

	//~ Begin USceneComponent Interface
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void UpdateBounds() override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End USceneComponent Interface

	//~ Begin IVoxelParameterOverridesOwner Interface
	virtual bool ShouldForceEnableOverride(const FVoxelParameterPath& Path) const override;
	virtual FVoxelParameterOverrides& GetParameterOverrides() override;
	//~ End IVoxelParameterOverridesOwner Interface

public:
	//~ Begin IVoxelRuntimeProvider Interface
	virtual TSharedPtr<FVoxelRuntime> GetRuntime() const final override
	{
		return Runtime;
	}
	//~ End IVoxelRuntimeProvider Interface

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	bool IsRuntimeCreated() const
	{
		return Runtime.IsValid();
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void QueueRecreate()
	{
		bRuntimeRecreateQueued = true;
	}

	// Will call CreateRuntime when it's ready to be created
	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void QueueCreateRuntime();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void CreateRuntime();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void DestroyRuntime();

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	virtual UVoxelGraph* GetGraph() const override
	{
		return Graph;
	}

	UFUNCTION(BlueprintCallable, Category = "Voxel")
	void SetGraph(UVoxelGraph* NewGraph);

private:
	bool bRuntimeCreateQueued = false;
	bool bRuntimeRecreateQueued = false;
	TSharedPtr<FVoxelRuntime> Runtime;
};