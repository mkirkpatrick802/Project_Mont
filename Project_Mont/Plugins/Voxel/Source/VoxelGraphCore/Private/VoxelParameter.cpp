// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelParameter.h"
#include "VoxelGraph.h"
#include "Channel/VoxelChannelAsset_DEPRECATED.h"

void FVoxelParameter::Fixup()
{
	if (Type.Is<FVoxelChannelRef_DEPRECATED>())
	{
		Type = FVoxelPinType::Make<FVoxelChannelName>();

		if (ensure(DeprecatedDefaultValue.IsObject()))
		{
			const UVoxelChannelAsset_DEPRECATED* Channel = Cast<UVoxelChannelAsset_DEPRECATED>(DeprecatedDefaultValue.GetObject());
			if (ensure(Channel))
			{
				DeprecatedDefaultValue = FVoxelPinValue::Make(Channel->MakeChannelName());
			}
		}
	}

	if (!Type.IsValid())
	{
		Type = FVoxelPinType::Make<float>();
	}
}