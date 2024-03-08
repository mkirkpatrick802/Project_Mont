// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "VoxelLandscapeActorFactories.generated.h"

UCLASS()
class UActorFactoryVoxelLandscape : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelLandscape();
};

UCLASS()
class UActorFactoryVoxelHeightBrush : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelHeightBrush();
};

UCLASS()
class UActorFactoryVoxelHeightBrush_FromStamp : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelHeightBrush_FromStamp();

	//~ Begin UActorFactory Interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	//~ End UActorFactory Interface
};

UCLASS()
class UActorFactoryVoxelVolumeBrush : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryVoxelVolumeBrush();
};