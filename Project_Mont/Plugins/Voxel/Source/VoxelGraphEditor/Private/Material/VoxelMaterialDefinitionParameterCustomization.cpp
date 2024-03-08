// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "Material/VoxelMaterialDefinition.h"
#include "Customizations/VoxelPinValueCustomizationHelper.h"

VOXEL_CUSTOMIZE_STRUCT_CHILDREN(FVoxelMaterialDefinitionParameter)(const TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FVoxelEditorUtilities::TrackHandle(PropertyHandle);

	ChildBuilder.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelMaterialDefinitionParameter, Name));

	const TSharedRef<IPropertyHandle> TypeHandle = PropertyHandle->GetChildHandleStatic(FVoxelMaterialDefinitionParameter, Type);
	TypeHandle->SetInstanceMetaData(STATIC_FNAME("FilterTypes"), "AllMaterials");
	ChildBuilder.AddProperty(TypeHandle);

	const TVoxelArray<TWeakObjectPtr<UVoxelMaterialDefinition>> MaterialDefinitions = FVoxelEditorUtilities::GetTypedOuters<UVoxelMaterialDefinition>(PropertyHandle);

	// Create category selection
	{
		const TSharedRef<IPropertyHandle> CategoryHandle = PropertyHandle->GetChildHandleStatic(FVoxelMaterialDefinitionParameter, Category);

		if (MaterialDefinitions.Num() == 0)
		{
			ChildBuilder.AddProperty(CategoryHandle);
		}
		else
		{
			FString Category;
			CategoryHandle->GetValue(Category);

			ChildBuilder.AddCustomRow(INVTEXT("Category"))
			.NameContent()
			[
				CategoryHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SBox)
				.MinDesiredWidth(125.f)
				[
					SNew(SVoxelDetailComboBox<FString>)
					.RefreshDelegate(this, ChildBuilder)
					.Options_Lambda([=]()
					{
						TSet<FString> Categories;
						for (const TWeakObjectPtr<UVoxelMaterialDefinition>& WeakMaterialDefinition : MaterialDefinitions)
						{
							const UVoxelMaterialDefinition* MaterialDefinition = WeakMaterialDefinition.Get();
							if (!ensureVoxelSlow(MaterialDefinition))
							{
								continue;
							}

							for (const auto& It : MaterialDefinition->GuidToMaterialParameter)
							{
								Categories.Add(It.Value.Category);
							}
						}
						Categories.Remove("");
						Categories.Add("Default");
						return Categories.Array();
					})
					.CurrentOption(Category.IsEmpty() ? "Default" : Category)
					.CanEnterCustomOption(true)
					.OptionText(MakeLambdaDelegate([](FString Option)
					{
						return Option;
					}))
					.OnSelection_Lambda([CategoryHandle](const FString NewValue)
					{
						CategoryHandle->SetValue(NewValue == "Default" ? "" : NewValue);
					})
				]
			];
		}
	}

	ChildBuilder.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelMaterialDefinitionParameter, Description));

	IDetailCategoryBuilder& DefaultValueCategory = ChildBuilder.GetParentCategory().GetParentLayout().EditCategory("Default Value", INVTEXT("Default Value"));
	DefaultValueCategory.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelMaterialDefinitionParameter, DefaultValue));

	IDetailCategoryBuilder& OptionsCategory = ChildBuilder.GetParentCategory().GetParentLayout().EditCategory("Options", INVTEXT("Options"));

	// Move Options category before Config and below all other categories
	OptionsCategory.SetSortOrder(9999);

	const auto GetMetaData = [](const TSharedPtr<IPropertyHandle>& Handle)
	{
		const FVoxelMaterialDefinitionParameter Parameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelMaterialDefinitionParameter>(Handle);
		return Parameter.MetaData;
	};
	const auto SetMetaData = [](const TSharedPtr<IPropertyHandle>& Handle, const TMap<FName, FString>& MetaData)
	{
		FVoxelMaterialDefinitionParameter Parameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelMaterialDefinitionParameter>(Handle);
		Parameter.MetaData = MetaData;
		FVoxelEditorUtilities::SetStructPropertyValue<FVoxelMaterialDefinitionParameter>(Handle, Parameter);
	};

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		OptionsCategory.AddCustomRow(INVTEXT("Slider Range")),
		PropertyHandle,
		INVTEXT("Slider Range"),
		INVTEXT("Allows setting the minimum and maximum values for the UI slider for this variable."),
		STATIC_FNAME("UIMin"),
		STATIC_FNAME("UIMax"),
		FVoxelPinValueCustomizationHelper::IsNumericType,
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		OptionsCategory.AddCustomRow(INVTEXT("Value Range")),
		PropertyHandle,
		INVTEXT("Value Range"),
		INVTEXT("The range of values allowed by this variable. Values outside of this will be clamped to the range."),
		STATIC_FNAME("ClampMin"),
		STATIC_FNAME("ClampMax"),
		FVoxelPinValueCustomizationHelper::IsNumericType,
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueUnitSetter(
		OptionsCategory.AddCustomRow(INVTEXT("Unit")),
		PropertyHandle,
		GetMetaData,
		SetMetaData);
}