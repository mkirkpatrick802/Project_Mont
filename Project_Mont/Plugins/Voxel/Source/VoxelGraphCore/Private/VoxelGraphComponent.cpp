// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphComponent.h"

void UVoxelGraphComponent::SetGraph(UVoxelGraph* NewGraph)
{
	if (Graph == NewGraph)
	{
		return;
	}
	Graph = NewGraph;

	NotifyGraphChanged();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphComponent::PostEditImport()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditImport();

	FixupParameterOverrides();
}

void UVoxelGraphComponent::PostInitProperties()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostInitProperties();

	FixupParameterOverrides();
}

#if WITH_EDITOR
void UVoxelGraphComponent::PostEditUndo()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditUndo();

	FixupParameterOverrides();
}

void UVoxelGraphComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(Graph))
	{
		NotifyGraphChanged();
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphComponent::ShouldForceEnableOverride(const FVoxelParameterPath& Path) const
{
	return true;
}

FVoxelParameterOverrides& UVoxelGraphComponent::GetParameterOverrides()
{
	return ParameterOverrides;
}