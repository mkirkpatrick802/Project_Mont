// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"

struct FVoxelMaterialDefinitionToolkit;

class FVoxelMaterialDefinitionParameterSelectionCustomization : public IDetailCustomization
{
public:
	const FGuid Guid;

	explicit FVoxelMaterialDefinitionParameterSelectionCustomization(const FGuid& Guid)
		: Guid(Guid)
	{
	}

	//~ Begin IDetailCustomization Interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	//~ End IDetailCustomization Interface

private:
	TSharedPtr<FVoxelStructDetailsWrapper> ParameterWrapper;
	TSharedPtr<FVoxelInstancedStructDetailsWrapper> ParameterDataWrapper;
};