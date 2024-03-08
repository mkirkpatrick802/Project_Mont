// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelTerminalGraph.h"

VOXEL_CUSTOMIZE_STRUCT_CHILDREN_RECURSIVE(FVoxelGraphProperty)(const TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	ChildBuilder.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelGraphProperty, Name));
	ChildBuilder.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelGraphProperty, Type));

	const TVoxelArray<TWeakObjectPtr<UVoxelTerminalGraph>> TerminalGraphs = FVoxelEditorUtilities::GetTypedOuters<UVoxelTerminalGraph>(PropertyHandle);

	// Create category selection
	{
		const TSharedRef<IPropertyHandle> CategoryHandle = PropertyHandle->GetChildHandleStatic(FVoxelGraphProperty, Category);

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
				.Options_Lambda([=]() -> TArray<FString>
				{
					TSet<FString> Categories;
					for (const TWeakObjectPtr<UVoxelTerminalGraph>& WeakTerminalGraph : TerminalGraphs)
					{
						const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
						if (!TerminalGraph)
						{
							continue;
						}

						for (const FGuid& Guid : TerminalGraph->GetInputs())
						{
							Categories.Add(TerminalGraph->FindInputChecked(Guid).Category);
						}
						for (const FGuid& Guid : TerminalGraph->GetOutputs())
						{
							Categories.Add(TerminalGraph->FindOutputChecked(Guid).Category);
						}
						for (const FGuid& Guid : TerminalGraph->GetLocalVariables())
						{
							Categories.Add(TerminalGraph->FindLocalVariableChecked(Guid).Category);
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

	ChildBuilder.AddProperty(PropertyHandle->GetChildHandleStatic(FVoxelGraphProperty, Description));

	const TSharedPtr<IPropertyHandle> DefaultValueHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_STATIC(FVoxelGraphInput, DefaultValue));
	if (!DefaultValueHandle)
	{
		// Not a FVoxelGraphInput
		return;
	}

	const FString* ShowDefaultValue = PropertyHandle->GetInstanceMetaData("ShowDefaultValue");
	if (!ensure(ShowDefaultValue))
	{
		return;
	}

	const bool bShowDefaultValue = *ShowDefaultValue == "true";
	ensure(bShowDefaultValue || *ShowDefaultValue == "false");

	if (!bShowDefaultValue)
	{
		return;
	}

	IDetailCategoryBuilder& DefaultValueCategory =
		ChildBuilder
		.GetParentCategory()
		.GetParentLayout()
		.EditCategory("Default Value", INVTEXT("Default Value"));

	// Move Default Value category before all options, after all categories
	DefaultValueCategory.SetSortOrder(9998);
	DefaultValueCategory.AddProperty(DefaultValueHandle.ToSharedRef());
}