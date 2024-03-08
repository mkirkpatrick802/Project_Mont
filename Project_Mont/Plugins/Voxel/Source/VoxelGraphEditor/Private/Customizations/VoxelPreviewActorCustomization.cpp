// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelActor.h"
#include "SculptVolume/VoxelSculptEdMode.h"
#include "VoxelParameterOverridesDetails.h"

VOXEL_CUSTOMIZE_CLASS(AVoxelPreviewActor)(IDetailLayoutBuilder& DetailLayout)
{
	FVoxelEditorUtilities::EnableRealtime();

	DetailLayout.AddPropertyToCategory(DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(AVoxelPreviewActor, TargetActor)));

	DetailLayout.HideCategory(STATIC_FNAME("TransformCommon"));
	DetailLayout.HideCategory(STATIC_FNAME("Actor"));
	DetailLayout.HideCategory(STATIC_FNAME("WorldPartition"));
	DetailLayout.HideCategory(STATIC_FNAME("LevelInstance"));

	const TSharedRef<IPropertyHandle> GraphHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(AVoxelPreviewActor, Graph_NewProperty));
	GraphHandle->MarkHiddenByCustomization();

	KeepAlive(FVoxelParameterOverridesDetails::Create(
		DetailLayout,
		GetObjectsBeingCustomized(DetailLayout),
		FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout)));

	// Hide component properties
	for (const FProperty& Property : GetClassProperties<UActorComponent>())
	{
		if (const TSharedPtr<IPropertyHandle> PropertyHandle = DetailLayout.GetProperty(Property.GetFName(), UActorComponent::StaticClass()))
		{
			PropertyHandle->MarkHiddenByCustomization();
		}
	}
}