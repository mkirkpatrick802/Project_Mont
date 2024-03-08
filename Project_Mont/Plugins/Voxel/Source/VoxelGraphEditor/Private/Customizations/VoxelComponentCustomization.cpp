// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelComponent.h"
#include "VoxelParameterOverridesDetails.h"

VOXEL_CUSTOMIZE_CLASS(UVoxelComponent)(IDetailLayoutBuilder& DetailLayout)
{
	FVoxelEditorUtilities::EnableRealtime();

	const TSharedRef<IPropertyHandle> GraphHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(UVoxelComponent, Graph_NewProperty));
	GraphHandle->MarkHiddenByCustomization();

	// Force graph at the bottom
	DetailLayout
	.EditCategory(
		"Config",
		{},
		ECategoryPriority::Uncommon)
	.AddProperty(GraphHandle);

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