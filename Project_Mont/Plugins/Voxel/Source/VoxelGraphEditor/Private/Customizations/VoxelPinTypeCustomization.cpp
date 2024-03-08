// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "Widgets/SVoxelGraphPinTypeComboBox.h"

class FVoxelPinTypeCustomization : public IPropertyTypeCustomization
{
public:
	virtual void CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		const TArray<TWeakObjectPtr<UObject>> SelectedObjects = CustomizationUtils.GetPropertyUtilities()->GetSelectedObjects();
		FString FilterTypes;
		if (const FString* InstanceFilterTypes = PropertyHandle->GetInstanceMetaData("FilterTypes"))
		{
			FilterTypes = *InstanceFilterTypes;
		}
		else
		{
			FilterTypes = PropertyHandle->GetMetaData("FilterTypes");
		}

		HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(SVoxelPinTypeComboBox)
			.AllowedTypes_Lambda([FilterTypes, SelectedObjects]
			{
				if (FilterTypes.IsEmpty())
				{
					return FVoxelPinTypeSet::All();
				}
				else if (FilterTypes == "AllUniforms")
				{
					return FVoxelPinTypeSet::AllUniforms();
				}
				else if (FilterTypes == "AllBuffers")
				{
					return FVoxelPinTypeSet::AllBuffers();
				}
				else if (FilterTypes == "AllUniformsAndBufferArrays")
				{
					return FVoxelPinTypeSet::AllUniformsAndBufferArrays();
				}
				else if (FilterTypes == "AllMaterials")
				{
					return FVoxelPinTypeSet::AllMaterials();
				}
				else
				{
					FVoxelPinTypeSet PinTypes = FVoxelPinTypeSet::All();
					for (TWeakObjectPtr<UObject> Object : SelectedObjects)
					{
						if (!ensure(Object.IsValid()))
						{
							continue;
						}

						UFunction* Function = Object->FindFunction(*FilterTypes);
						if (!ensure(Function) ||
							!ensure(!Function->Children) ||
							!ensure(Function->ParmsSize == sizeof(PinTypes)))
						{
							continue;
						}

						Object->ProcessEvent(Function, &PinTypes);
					}
					return PinTypes;
				}
			})
			.OnTypeChanged_Lambda([=](const FVoxelPinType& Type)
			{
				FVoxelEditorUtilities::SetStructPropertyValue(PropertyHandle, Type);
			})
			.CurrentType_Lambda([WeakHandle = MakeWeakPtr(PropertyHandle)]() -> FVoxelPinType
			{
				const TSharedPtr<IPropertyHandle> Handle = WeakHandle.Pin();
				if (!Handle)
				{
					return {};
				}

				return FVoxelEditorUtilities::GetStructPropertyValue<FVoxelPinType>(Handle);
			})
		];
	}

	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
	}
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelPinType, FVoxelPinTypeCustomization);