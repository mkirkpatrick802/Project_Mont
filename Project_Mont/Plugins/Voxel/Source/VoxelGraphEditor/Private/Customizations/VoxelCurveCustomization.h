// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Curves/RichCurve.h"

class SVoxelCurveEditor;
class SVoxelCurveThumbnail;

class FVoxelCurveCustomization
	: public IPropertyTypeCustomization
	, public FVoxelTicker
{
public:
	//~ Begin IPropertyTypeCustomization Interface
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void Tick() override;
	//~ End IPropertyTypeCustomization Interface

private:
	TSharedPtr<IPropertyHandle> CurveHandle;

	TSharedPtr<SVoxelCurveEditor> CurveEditor;
	FRichCurve CachedCurve;
	TSharedPtr<SVoxelCurveThumbnail> CurveThumbnailWidget;
};