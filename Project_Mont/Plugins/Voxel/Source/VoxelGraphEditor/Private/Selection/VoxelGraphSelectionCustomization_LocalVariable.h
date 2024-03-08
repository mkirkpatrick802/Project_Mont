﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

class FVoxelGraphSelectionCustomization_LocalVariable : public FVoxelDetailCustomization
{
public:
	const FGuid Guid;

	explicit FVoxelGraphSelectionCustomization_LocalVariable(const FGuid& Guid)
		: Guid(Guid)
	{
	}

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface
};