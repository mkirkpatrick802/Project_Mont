// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "Nodes/VoxelRandomNodes.h"

class FVoxelRandomVectorSettingsCustomization : public IPropertyTypeCustomization
{
public:
	virtual void CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		const TSharedRef<IPropertyHandle> TypeHandle = PropertyHandle->GetChildHandleStatic(FVoxelRandomVectorSettings, Type);
		HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			TypeHandle->CreatePropertyValueWidget()
		];
	}

	virtual void CustomizeChildren(const TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		const TSharedRef<IPropertyHandle> TypeHandle = PropertyHandle->GetChildHandleStatic(FVoxelRandomVectorSettings, Type);
		const TSharedRef<IPropertyHandle> RangeXHandle = PropertyHandle->GetChildHandleStatic(FVoxelRandomVectorSettings, RangeX);
		const TSharedRef<IPropertyHandle> RangeYHandle = PropertyHandle->GetChildHandleStatic(FVoxelRandomVectorSettings, RangeY);
		const TSharedRef<IPropertyHandle> RangeZHandle = PropertyHandle->GetChildHandleStatic(FVoxelRandomVectorSettings, RangeZ);

		IDetailPropertyRow& RangeXRow = ChildBuilder.AddProperty(RangeXHandle);

		TSharedPtr<SWidget> DummyNameWidget;
		TSharedPtr<SWidget> ValueWidget;
		RangeXRow.GetDefaultWidgets(DummyNameWidget, ValueWidget, true);

		RangeXRow.CustomWidget(true)
		.NameContent()
		[
			SNew(SVoxelDetailText)
			.Text_Lambda([=]
			{
				switch (FVoxelEditorUtilities::GetEnumPropertyValue<EVoxelRandomVectorType>(TypeHandle))
				{
				default: return INVTEXT("Invalid");
				case EVoxelRandomVectorType::Uniform: return INVTEXT("Range");
				case EVoxelRandomVectorType::Free: return INVTEXT("Range X");
				case EVoxelRandomVectorType::LockXY: return INVTEXT("Range XY");
				}
			})
		]
		.ValueContent()
		.MinDesiredWidth(251.0f)
		.MaxDesiredWidth(251.0f)
		[
			ValueWidget.ToSharedRef()
		];

		ChildBuilder.AddProperty(RangeYHandle)
		.Visibility(MakeAttributeLambda([=]
		{
			return FVoxelEditorUtilities::GetEnumPropertyValue<EVoxelRandomVectorType>(TypeHandle) == EVoxelRandomVectorType::Free ? EVisibility::Visible : EVisibility::Collapsed;
		}));

		ChildBuilder.AddProperty(RangeZHandle)
		.Visibility(MakeAttributeLambda([=]
		{
			return FVoxelEditorUtilities::GetEnumPropertyValue<EVoxelRandomVectorType>(TypeHandle) != EVoxelRandomVectorType::Uniform ? EVisibility::Visible : EVisibility::Collapsed;
		}));
	}
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelRandomVectorSettings, FVoxelRandomVectorSettingsCustomization);