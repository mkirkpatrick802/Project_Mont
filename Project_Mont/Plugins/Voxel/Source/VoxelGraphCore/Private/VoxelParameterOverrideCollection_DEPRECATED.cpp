// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelParameterOverrideCollection_DEPRECATED.h"
#include "VoxelParameterOverridesOwner.h"
#include "Channel/VoxelChannelAsset_DEPRECATED.h"

void FVoxelParameterOverrideCollection_DEPRECATED::MigrateTo(IVoxelParameterOverridesOwner& OverridesOwner)
{
	VOXEL_FUNCTION_COUNTER();

	TMap<FVoxelParameterPath, FVoxelParameterValueOverride>& PathToValueOverride = OverridesOwner.GetPathToValueOverride();

	// If all parameters are disabled, this is likely an old file
	// Enable all parameters by default
	bool bEnableAll = true;
	for (const FVoxelParameterOverride_DEPRECATED& Parameter : Parameters)
	{
		if (Parameter.bEnable)
		{
			bEnableAll = false;
		}
	}

	for (const FVoxelParameterOverride_DEPRECATED& Parameter : Parameters)
	{
		FVoxelParameterPath Path;
		Path.Guids.Add(Parameter.Parameter.DeprecatedGuid);

		FVoxelParameterValueOverride ValueOverride;
		ValueOverride.bEnable = bEnableAll ? true : Parameter.bEnable;
		ValueOverride.Value = Parameter.ValueOverride;
#if WITH_EDITOR
		ValueOverride.CachedName = Parameter.Parameter.Name;
		ValueOverride.CachedCategory = Parameter.Parameter.Category;
#endif

		if (ValueOverride.Value.Is<UVoxelChannelAsset_DEPRECATED>())
		{
			if (UVoxelChannelAsset_DEPRECATED* Channel = ValueOverride.Value.Get<UVoxelChannelAsset_DEPRECATED>())
			{
				ValueOverride.Value = FVoxelPinValue::Make<FVoxelChannelName>(Channel->MakeChannelName());
			}
		}

		ensure(!PathToValueOverride.Contains(Path));
		PathToValueOverride.Add(Path, ValueOverride);
	}

	OverridesOwner.FixupParameterOverrides();

	if (bEnableAll)
	{
		// Disable any unneeded override

		for (auto& It : PathToValueOverride)
		{
			It.Value.bEnable = false;
		}

		OverridesOwner.FixupParameterOverrides();

		for (auto& It : PathToValueOverride)
		{
			It.Value.bEnable = true;
		}
	}

	Parameters = {};
}