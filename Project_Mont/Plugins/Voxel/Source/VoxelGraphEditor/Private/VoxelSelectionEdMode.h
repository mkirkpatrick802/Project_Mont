// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Tools/UEdMode.h"
#include "Tools/LegacyEdModeInterfaces.h"
#include "VoxelSelectionEdMode.generated.h"

UCLASS()
class UVoxelSelectionEdModeSettings : public UObject
{
	GENERATED_BODY()
};

UCLASS()
class UVoxelSelectionEdMode : public UEdMode, public ILegacyEdModeViewportInterface
{
	GENERATED_BODY()

public:
	UVoxelSelectionEdMode();

	//~ Begin UEdMode Interface
	virtual bool IsCompatibleWith(FEditorModeID OtherModeID) const override
	{
		return true;
	}
	virtual bool UsesToolkits() const override
	{
		return false;
	}

	virtual bool HandleClick(FEditorViewportClient* ViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	//~ End UEdMode Interface
};