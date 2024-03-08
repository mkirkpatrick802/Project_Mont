// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "VoxelPinValue.h"
#include "Customizations/VoxelPinValueCustomizationHelper.h"

class FVoxelPinValueCustomization
	: public IPropertyTypeCustomization
	, public FVoxelTicker
{
public:
	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(
		const TSharedRef<IPropertyHandle> PropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		TypeHandle = PropertyHandle->GetChildHandleStatic(FVoxelPinValue, Type);
		CachedType = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelPinType>(TypeHandle);
		RefreshDelegate = FVoxelEditorUtilities::MakeRefreshDelegate(this, CustomizationUtils);

		if (!CachedType.IsValid())
		{
			HeaderRow
			.NameContent()
			[
				PropertyHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			[
				SNew(SVoxelDetailText)
				.Text(INVTEXT("Invalid type"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
		}
	}

	virtual void CustomizeChildren(
		const TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override
	{
		if (const TSharedPtr<IPropertyHandle> ParentHandle = PropertyHandle->GetParentHandle())
		{
			if (ParentHandle->AsArray())
			{
				for (const auto& It : *ParentHandle->GetInstanceMetaDataMap())
				{
					PropertyHandle->SetInstanceMetaData(It.Key, It.Value);
				}
			}
		}

		Wrapper = FVoxelPinValueCustomizationHelper::CreatePinValueCustomization(
			PropertyHandle,
			ChildBuilder,
			FVoxelEditorUtilities::MakeRefreshDelegate(this, CustomizationUtils),
			{},
			[&](FDetailWidgetRow& Row, const TSharedRef<SWidget>& ValueWidget)
			{
				const FVoxelPinType Type = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelPinType>(TypeHandle);
				const float Width = FVoxelPinValueCustomizationHelper::GetValueWidgetWidthByType(PropertyHandle, Type);

				Row
				.NameContent()
				[
					PropertyHandle->CreatePropertyNameWidget()
				]
				.ValueContent()
				.MinDesiredWidth(Width)
				.MaxDesiredWidth(Width)
				[
					ValueWidget
				];
			},
			// Used to load/save expansion state
			FAddPropertyParams().UniqueId("FVoxelPinValueCustomization"));
	}
	//~ End IPropertyTypeCustomization Interface

	//~ Begin FVoxelTicker Interface
	virtual void Tick() override
	{
		if (!TypeHandle ||
			TypeHandle->GetNumPerObjectValues() == 0)
		{
			return;
		}

		const FVoxelPinType Type = FVoxelEditorUtilities::GetStructPropertyValue<FVoxelPinType>(TypeHandle);
		if (Type != CachedType)
		{
			RefreshDelegate.ExecuteIfBound();
			TypeHandle = nullptr;
		}
	}
	//~ End FVoxelTicker Interface

private:
	TSharedPtr<FVoxelInstancedStructDetailsWrapper> Wrapper;

	TSharedPtr<IPropertyHandle> TypeHandle;
	FVoxelPinType CachedType;
	FSimpleDelegate RefreshDelegate;
};

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelPinValue, FVoxelPinValueCustomization);
DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelTerminalPinValue, FVoxelPinValueCustomization);