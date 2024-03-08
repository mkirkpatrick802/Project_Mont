// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelSurface.h"

class FVoxelSurfaceAttributeInfoCustomization : public IPropertyTypeCustomization
{
public:
	virtual void CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		const TSharedRef<IPropertyHandle> NameHandle = PropertyHandle->GetChildHandleStatic(FVoxelSurfaceAttributeInfo, Name);

		HeaderRow
		.NameContent()
		.MinDesiredWidth(125.f)
		.MaxDesiredWidth(125.f)
		[
			SNew(SBox)
			.MinDesiredWidth(125.f)
			.MaxDesiredWidth(125.f)
			[
				NameHandle->CreatePropertyValueWidget()
			]
		]
		.ValueContent()
		[
			SAssignNew(TypeWidget, SBox)
		];
	}

	virtual void CustomizeChildren(const TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		const TSharedRef<IPropertyHandle> TypeHandle = PropertyHandle->GetChildHandleStatic(FVoxelSurfaceAttributeInfo, Type);
		IDetailPropertyRow& HandleRow = ChildBuilder.AddProperty(TypeHandle);

		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		HandleRow.GetDefaultWidgets(NameWidget, ValueWidget);

		HandleRow.Visibility(EVisibility::Collapsed);

		TypeWidget->SetContent(ValueWidget.ToSharedRef());
	}

private:
	TSharedPtr<SBox> TypeWidget;
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelSurfaceAttributeInfo, FVoxelSurfaceAttributeInfoCustomization);