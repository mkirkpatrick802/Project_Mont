// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class FVoxelGraphPreviewSettingsCustomization : public IDetailCustomization
{
public:
	FVoxelGraphPreviewSettingsCustomization() = default;

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

private:
	TSharedPtr<FVoxelInstancedStructDetailsWrapper> Wrapper;
};