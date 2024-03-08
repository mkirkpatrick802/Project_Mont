// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Channel/VoxelChannelDefinition.h"

FString FVoxelChannelDefinition::ToString() const
{
	return FString::Printf(TEXT("%s Type=%s Default=%s"),
		*Name.ToString(),
		*Type.ToString(),
		*GetExposedDefaultValue().ExportToString());
}

FVoxelPinValue FVoxelChannelDefinition::GetExposedDefaultValue() const
{
	return FVoxelPinType::MakeExposedValue(DefaultValue, Type.IsBufferArray());
}