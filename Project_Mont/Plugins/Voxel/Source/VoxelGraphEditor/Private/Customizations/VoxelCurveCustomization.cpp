// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCurveCustomization.h"
#include "Widgets/SVoxelCurveThumbnail.h"
#include "CurveEditor/SVoxelCurveEditor.h"
#include "FunctionLibrary/VoxelCurveFunctionLibrary.h"

void FVoxelCurveCustomization::CustomizeHeader(const TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	CurveHandle = PropertyHandle->GetChildHandleStatic(FVoxelCurve, Curve);

	TSharedPtr<SWidget> ValueWidget;
	if (PropertyHandle->GetNumPerObjectValues() > 1)
	{
		ValueWidget =
			SNew(SVoxelDetailText)
			.Text(INVTEXT("Multiple Values"));
	}
	else
	{
		CachedCurve = FVoxelEditorUtilities::GetStructPropertyValue<FRichCurve>(CurveHandle);

		const FRichCurve& Curve = FVoxelEditorUtilities::GetStructPropertyValue<FRichCurve>(CurveHandle);
		CurveThumbnailWidget = SNew(SVoxelCurveThumbnail, &Curve);

		ValueWidget = CurveThumbnailWidget;
	}

	HeaderRow
	.ShouldAutoExpand(true)
	.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		ValueWidget.ToSharedRef()
	];
}

void FVoxelCurveCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	if (PropertyHandle->GetNumPerObjectValues() > 1)
	{
		return;
	}

	ChildBuilder.AddCustomRow(INVTEXT("Curve"))
	.WholeRowContent().VAlign(VAlign_Fill)
	[
		SAssignNew(CurveEditor, SVoxelCurveEditor)
		.Handle(CurveHandle)
	];
}

void FVoxelCurveCustomization::Tick()
{
	if (CurveHandle->GetNumPerObjectValues() > 1)
	{
		return;
	}

	const FRichCurve* NewCurve = FVoxelEditorUtilities::TryGetStructPropertyValue<FRichCurve>(CurveHandle);
	if (!NewCurve)
	{
		// Can fail since we query on tick
		return;
	}

	if (CachedCurve == *NewCurve)
	{
		return;
	}

	CachedCurve = *NewCurve;
	CurveThumbnailWidget->UpdateCurve(&CachedCurve);
	CurveEditor->RefreshCurve();
}

DEFINE_VOXEL_STRUCT_LAYOUT(FVoxelCurve, FVoxelCurveCustomization);