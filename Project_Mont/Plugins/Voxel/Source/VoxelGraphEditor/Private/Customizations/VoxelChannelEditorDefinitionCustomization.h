// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"

class FVoxelChannelEditorDefinitionCustomization : public IDetailCustomization
{
public:
	explicit FVoxelChannelEditorDefinitionCustomization(const FVoxelPinType ChannelType)
		: ChannelType(ChannelType)
	{}

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

public:
	const FVoxelPinType ChannelType;
};