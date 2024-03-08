// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionParameter.h"

void FVoxelMaterialDefinitionParameter::Fixup()
{
	if (!Type.IsValid())
	{
		Type = FVoxelPinType::Make<float>();
	}

	DefaultValue.Fixup(Type);
}