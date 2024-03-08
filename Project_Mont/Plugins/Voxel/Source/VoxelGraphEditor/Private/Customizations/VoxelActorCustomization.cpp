// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelActor.h"
#include "Sculpt/VoxelSculptStorage.h"
#include "SculptVolume/VoxelSculptEdMode.h"
#include "VoxelParameterOverridesDetails.h"

VOXEL_CUSTOMIZE_CLASS(AVoxelActor)(IDetailLayoutBuilder& DetailLayout)
{
	if (DetailLayout.GetBaseClass()->IsChildOf<AVoxelPreviewActor>())
	{
		return;
	}

	FVoxelEditorUtilities::EnableRealtime();

	const TVoxelArray<TWeakObjectPtr<AVoxelActor>> WeakActors = GetWeakObjectsBeingCustomized(DetailLayout);

	{
		DetailLayout.EditCategory(STATIC_FNAME("Sculpt"))
		.AddCustomRow(INVTEXT("Sculpt"))
		.NameContent()
		[
			SNew(SVoxelDetailText)
			.Text(INVTEXT("Clear Sculpt Data"))
		]
		.ValueContent()
		[
			SNew(SVoxelDetailButton)
			.Text(INVTEXT("Clear"))
			.ToolTipText(INVTEXT("Clear all the sculpt data stored in this actor"))
			.OnClicked_Lambda([=]
			{
				FScopedTransaction Transaction(TEXT("Clear Sculpt Data"), INVTEXT("Clear Sculpt Data"), nullptr);

				for (const TWeakObjectPtr<AVoxelActor>& WeakActor : WeakActors)
				{
					if (!WeakActor.IsValid() ||
						!ensure(WeakActor->SculptStorageComponent))
					{
						continue;
					}

					WeakActor->SculptStorageComponent->ClearData();
				}

				return FReply::Handled();
			})
		];
	}

	const TSharedRef<IPropertyHandle> GraphHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_STATIC(AVoxelActor, Graph_NewProperty));
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

	const auto MoveToDefaultCategory = [&](const FName CategoryName, const TSet<FName>& ExplicitProperties = {}, const bool bCreateGroup = true)
	{
		IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory(CategoryName);
		TArray<TSharedRef<IPropertyHandle>> Properties;
		CategoryBuilder.GetDefaultProperties(Properties);

		IDetailCategoryBuilder& DefaultCategory = DetailLayout.EditCategory(STATIC_FNAME("Default"), {}, ECategoryPriority::Uncommon);

		DetailLayout.HideCategory(CategoryName);

		if (!bCreateGroup)
		{
			for (const TSharedRef<IPropertyHandle>& Property : Properties)
			{
				if (ExplicitProperties.Num() == 0 ||
					ExplicitProperties.Contains(Property->GetProperty()->GetFName()))
				{
					DefaultCategory.AddProperty(Property);
				}
			}
			return;
		}

		IDetailGroup& Group = DefaultCategory.AddGroup(CategoryName, CategoryBuilder.GetDisplayName());
		for (const TSharedRef<IPropertyHandle>& Property : Properties)
		{
			if (ExplicitProperties.Num() == 0 ||
				ExplicitProperties.Contains(Property->GetProperty()->GetFName()))
			{
				Group.AddPropertyRow(Property);
			}
		}
	};

	MoveToDefaultCategory(STATIC_FNAME("Actor"), { GET_MEMBER_NAME_STATIC(AActor, Tags) }, false);
	MoveToDefaultCategory(STATIC_FNAME("WorldPartition"));
	MoveToDefaultCategory(STATIC_FNAME("LevelInstance"));
}