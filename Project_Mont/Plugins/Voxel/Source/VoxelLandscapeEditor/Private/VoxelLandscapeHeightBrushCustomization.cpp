// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "Height/VoxelLandscapeHeightBrush.h"

VOXEL_CUSTOMIZE_CLASS(AVoxelLandscapeHeightBrush)(IDetailLayoutBuilder& DetailLayout)
{
	FVoxelEditorUtilities::EnableRealtime();

#if 0
	TVoxelArray<IVoxelParameterOverridesOwner*> ParameterOverridesOwners;
	for (const AVoxelLandscapeHeightBrush* Brush : GetObjectsBeingCustomized(DetailLayout))
	{
		ParameterOverridesOwners.Add(Brush->GraphComponent.Get());
	}

	KeepAlive(FVoxelParameterOverridesDetails::Create(
		DetailLayout.EditCategory("Height"),
		ParameterOverridesOwners,
		FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout)));

	// Hide component properties
	for (const FProperty& Property : GetClassProperties<UActorComponent>())
	{
		if (const TSharedPtr<IPropertyHandle> PropertyHandle = DetailLayout.GetProperty(Property.GetFName(), UActorComponent::StaticClass()))
		{
			PropertyHandle->MarkHiddenByCustomization();
		}
	}

	const auto MoveToMiscCategory = [&](const FName CategoryName, const TSet<FName>& ExplicitProperties = {}, const bool bCreateGroup = true)
	{
		IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory(CategoryName);
		TArray<TSharedRef<IPropertyHandle>> Properties;
		CategoryBuilder.GetDefaultProperties(Properties);

		IDetailCategoryBuilder& MiscCategory = DetailLayout.EditCategory(STATIC_FNAME("Misc"), {}, ECategoryPriority::Uncommon);

		DetailLayout.HideCategory(CategoryName);

		if (!bCreateGroup)
		{
			for (const TSharedRef<IPropertyHandle>& Property : Properties)
			{
				if (ExplicitProperties.Num() == 0 ||
					ExplicitProperties.Contains(Property->GetProperty()->GetFName()))
				{
					MiscCategory.AddProperty(Property);
				}
			}
			return;
		}

		IDetailGroup& Group = MiscCategory.AddGroup(CategoryName, CategoryBuilder.GetDisplayName());
		for (const TSharedRef<IPropertyHandle>& Property : Properties)
		{
			if (ExplicitProperties.Num() == 0 ||
				ExplicitProperties.Contains(Property->GetProperty()->GetFName()))
			{
				Group.AddPropertyRow(Property);
			}
		}
	};

	MoveToMiscCategory(STATIC_FNAME("Actor"), { GET_MEMBER_NAME_STATIC(AActor, Tags) }, false);
	MoveToMiscCategory(STATIC_FNAME("WorldPartition"));
	MoveToMiscCategory(STATIC_FNAME("LevelInstance"));
#endif
}