// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelRuntimeProvider.h"
#include "VoxelRuntimeParameter.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelParameterOverrideCollection_DEPRECATED.h"
#include "VoxelActor.generated.h"

class FVoxelRuntime;
class UVoxelGraph;
class UVoxelPointStorage;
class UVoxelSculptStorage;
class UVoxelParameterContainer_DEPRECATED;

UCLASS()
class VOXELGRAPHCORE_API UVoxelActorRootComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelActorRootComponent();

	//~ Begin UPrimitiveComponent Interface
	virtual void UpdateBounds() override;
	//~ End UPrimitiveComponent Interface
};

UCLASS(HideCategories = ("Rendering", "Replication", "Input", "Collision", "LOD", "HLOD", "Cooking", "DataLayers", "Networking", "Physics"))
class VOXELGRAPHCORE_API AVoxelActor
	: public AActor
	, public IVoxelRuntimeProvider
	, public IVoxelParameterOverridesObjectOwner
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Voxel")
	bool bCreateRuntimeOnBeginPlay = true;

#if WITH_EDITOR
	bool bCreateRuntimeOnConstruction_EditorOnly = true;
#endif

	FSimpleMulticastDelegate OnRuntimeCreated;
	FSimpleMulticastDelegate OnRuntimeDestroyed;

protected:
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

	friend class FAVoxelActorCustomization;

public:
	UPROPERTY()
	TObjectPtr<UVoxelPointStorage> PointStorageComponent;

	UPROPERTY()
	TObjectPtr<UVoxelSculptStorage> SculptStorageComponent;

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
	AVoxelActor();
	virtual ~AVoxelActor() override;

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void BeginDestroy() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostLoad() override;
	virtual void PostEditImport() override;
	virtual void PostInitProperties() override;
	virtual void PostCDOContruct() override;

#if WITH_EDITOR
	virtual bool Modify(bool bAlwaysMarkDirty = true) override;
	virtual void PostEditUndo() override;
	virtual void PreEditChange(FProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void Tick(float DeltaTime) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	//~ End AActor Interface

	//~ Begin IVoxelParameterOverridesOwner Interface
	virtual bool ShouldForceEnableOverride(const FVoxelParameterPath& Path) const override;
	virtual FVoxelParameterOverrides& GetParameterOverrides() override;
	//~ End IVoxelParameterOverridesOwner Interface

	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

public:
	FVoxelRuntimeParameters DefaultRuntimeParameters;

	virtual FVoxelRuntimeParameters GetRuntimeParameters() const;

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

private:
	bool bDisableModify = false;

	friend FVoxelRuntime;
};