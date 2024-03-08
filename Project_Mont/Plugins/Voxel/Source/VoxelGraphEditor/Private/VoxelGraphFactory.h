// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphFactory.generated.h"

class UVoxelGraph;

UCLASS()
class UVoxelGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	UVoxelGraphFactory();

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ End UFactory Interface

public:
	UPROPERTY()
	TObjectPtr<UVoxelGraph> AssetToCopy;
};