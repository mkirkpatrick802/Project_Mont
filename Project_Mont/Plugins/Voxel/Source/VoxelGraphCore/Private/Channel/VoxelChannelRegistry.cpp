// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Channel/VoxelChannelRegistry.h"
#include "Channel/VoxelChannelManager.h"
#include "Buffer/VoxelBaseBuffers.h"

DEFINE_VOXEL_FACTORY(UVoxelChannelRegistry);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChannelExposedDefinition::Fixup()
{
	if (!Type.IsValid())
	{
		Type = FVoxelPinType::Make<FVoxelFloatBuffer>();
	}

	DefaultValue.Fixup(Type.GetExposedType());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelChannelRegistry::UpdateChannels()
{
	VOXEL_FUNCTION_COUNTER();

	TSet<FName> UsedNames;
	for (FVoxelChannelExposedDefinition& Channel : Channels)
	{
		if (Channel.Name.IsNone())
		{
			Channel.Name = "MyChannel";
		}

		while (UsedNames.Contains(Channel.Name))
		{
			Channel.Name.SetNumber(Channel.Name.GetNumber() + 1);
		}

		UsedNames.Add(Channel.Name);

		Channel.Fixup();
	}

	GVoxelChannelManager->UpdateChannelsFromAsset_GameThread(
		this,
		GetName(),
		Channels);
}

void UVoxelChannelRegistry::PostLoad()
{
	Super::PostLoad();

	if (!IsTemplate())
	{
		UpdateChannels();
	}
}

void UVoxelChannelRegistry::BeginDestroy()
{
	if (!IsTemplate())
	{
		GVoxelChannelManager->RemoveChannelsFromAsset_GameThread(this);
	}

	Super::BeginDestroy();
}

#if WITH_EDITOR
void UVoxelChannelRegistry::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Interactive)
	{
		return;
	}

	UpdateChannels();
}
#endif