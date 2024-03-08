// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "VoxelInlineGraph.h"
#include "FunctionLibrary/VoxelCurveFunctionLibrary.h"
#include "Customizations/VoxelPinValueCustomizationHelper.h"

VOXEL_CUSTOMIZE_STRUCT_CHILDREN(FVoxelParameter)(const TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FVoxelEditorUtilities::TrackHandle(PropertyHandle);

	ChildBuilder.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelParameter, Name));

	const TSharedRef<IPropertyHandle> TypeHandle = PropertyHandle->GetChildHandleStatic(FVoxelParameter, Type);
	if (PropertyHandle->HasMetaData(STATIC_FNAME("FilterTypes")))
	{
		TypeHandle->SetInstanceMetaData(STATIC_FNAME("FilterTypes"), PropertyHandle->GetMetaData(STATIC_FNAME("FilterTypes")));
	}
	ChildBuilder.AddProperty(TypeHandle);

	const TVoxelArray<TWeakObjectPtr<UVoxelGraph>> Graphs = FVoxelEditorUtilities::GetTypedOuters<UVoxelGraph>(PropertyHandle);

	// Create category selection
	{
		const TSharedRef<IPropertyHandle> CategoryHandle = PropertyHandle->GetChildHandleStatic(FVoxelParameter, Category);

		if (Graphs.Num() == 0)
		{
			ChildBuilder.AddProperty(CategoryHandle);
		}
		else
		{
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
						for (const TWeakObjectPtr<UVoxelGraph>& WeakGraph : Graphs)
						{
							const UVoxelGraph* Graph = WeakGraph.Get();
							if (!ensureVoxelSlow(Graph))
							{
								continue;
							}

							for (const FGuid& Guid : Graph->GetParameters())
							{
								Categories.Add(Graph->FindParameterChecked(Guid).Category);
							}
						}
						Categories.Remove("");
						Categories.Add("Default");
						return Categories.Array();
					})
					.CurrentOption_Lambda([=]
					{
						FString Category;
						CategoryHandle->GetValue(Category);
						return Category.IsEmpty() ? "Default" : Category;
					})
					.CanEnterCustomOption(true)
					.OptionText(MakeLambdaDelegate([](FString Option)
					{
						return Option;
					}))
					.OnSelection_Lambda([CategoryHandle](FString NewValue)
					{
						CategoryHandle->SetValue(NewValue == "Default" ? "" : NewValue);
					})
				]
			];
		}
	}

	ChildBuilder.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelParameter, Description));

	IDetailCategoryBuilder& DefaultValueCategory = ChildBuilder.GetParentCategory().GetParentLayout().EditCategory("Options", INVTEXT("Options"));

	// Move Options category below all other categories
	DefaultValueCategory.SetSortOrder(9999);

	const auto GetMetaData = [](const TSharedPtr<IPropertyHandle>& Handle)
	{
		const FVoxelParameter Parameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelParameter>(Handle);
		return Parameter.MetaData;
	};
	const auto SetMetaData = [](const TSharedPtr<IPropertyHandle>& Handle, const TMap<FName, FString>& MetaData)
	{
		FVoxelParameter Parameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelParameter>(Handle);
		Parameter.MetaData = MetaData;
		FVoxelEditorUtilities::SetStructPropertyValue<FVoxelParameter>(Handle, Parameter);
	};

	FVoxelPinValueCustomizationHelper::CreatePinValueChannelTypeSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Channel Type")),
		PropertyHandle,
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Slider Range")),
		PropertyHandle,
		INVTEXT("Slider Range"),
		INVTEXT("Allows setting the minimum and maximum values for the UI slider for this variable."),
		STATIC_FNAME("UIMin"),
		STATIC_FNAME("UIMax"),
		FVoxelPinValueCustomizationHelper::IsNumericType,
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Value Range")),
		PropertyHandle,
		INVTEXT("Value Range"),
		INVTEXT("The range of values allowed by this variable. Values outside of this will be clamped to the range."),
		STATIC_FNAME("ClampMin"),
		STATIC_FNAME("ClampMax"),
		FVoxelPinValueCustomizationHelper::IsNumericType,
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Horizontal UI Range")),
		PropertyHandle,
		INVTEXT("Horizontal UI Range"),
		INVTEXT("The horizontal range of visible curve editor."),
		STATIC_FNAME("UIHorizontalMin"),
		STATIC_FNAME("UIHorizontalMax"),
		[](const FVoxelPinType& Type)
		{
			return Type.Is<FVoxelCurve>();
		},
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Vertical UI Range")),
		PropertyHandle,
		INVTEXT("Vertical UI Range"),
		INVTEXT("The vertical range of visible curve editor."),
		STATIC_FNAME("UIVerticalMin"),
		STATIC_FNAME("UIVerticalMax"),
		[](const FVoxelPinType& Type)
		{
			return Type.Is<FVoxelCurve>();
		},
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Horizontal Value Clamp")),
		PropertyHandle,
		INVTEXT("Horizontal Value Clamp"),
		INVTEXT("The horizontal range of possible values."),
		STATIC_FNAME("ClampHorizontalMin"),
		STATIC_FNAME("ClampHorizontalMax"),
		[](const FVoxelPinType& Type)
		{
			return Type.Is<FVoxelCurve>();
		},
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueRangeSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Vertical Value Clamp")),
		PropertyHandle,
		INVTEXT("Vertical Value Clamp"),
		INVTEXT("The vertical range possible values."),
		STATIC_FNAME("ClampVerticalMin"),
		STATIC_FNAME("ClampVerticalMax"),
		[](const FVoxelPinType& Type)
		{
			return Type.Is<FVoxelCurve>();
		},
		GetMetaData,
		SetMetaData);

	FVoxelPinValueCustomizationHelper::CreatePinValueUnitSetter(
		DefaultValueCategory.AddCustomRow(INVTEXT("Unit")),
		PropertyHandle,
		GetMetaData,
		SetMetaData);

	const FVoxelPinType* Type = FVoxelEditorUtilities::TryGetStructPropertyValue<FVoxelPinType>(TypeHandle);
	if (!Type ||
		!Type->Is<FVoxelInlineGraph>())
	{
		return;
	}

	ChildBuilder.AddCustomRow(INVTEXT("BaseGraph"))
	.NameContent()
	[
		SNew(SVoxelDetailText)
		.Text(INVTEXT("Base Graph"))
	]
	.ValueContent()
	[
		SNew(SObjectPropertyEntryBox)
		.AllowedClass(UVoxelGraph::StaticClass())
		.AllowClear(true)
		.ThumbnailPool(FVoxelEditorUtilities::GetThumbnailPool())
		.ObjectPath_Lambda([=]
		{
			const FVoxelParameter Parameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelParameter>(PropertyHandle);
			return Parameter.MetaData.FindRef("BaseGraph");
		})
		.OnObjectChanged_Lambda([=](const FAssetData& NewAssetData)
		{
			FVoxelParameter Parameter = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelParameter>(PropertyHandle);
			Parameter.MetaData.Add("BaseGraph", NewAssetData.GetSoftObjectPath().ToString());
			FVoxelEditorUtilities::SetStructPropertyValue<FVoxelParameter>(PropertyHandle, Parameter);
		})
	];
}