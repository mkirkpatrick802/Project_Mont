// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Selection/VoxelGraphSelectionCustomization_Parameter.h"
#include "VoxelGraph.h"
#include "VoxelParameterOverridesDetails.h"

void FVoxelGraphSelectionCustomization_Parameter::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	if (!Guid.IsValid())
	{
		return;
	}

	const TSharedRef<FVoxelStructDetailsWrapper> Wrapper = FVoxelStructDetailsWrapper::Make<UVoxelGraph, FVoxelParameter>(
		DetailLayout,
		[Guid = Guid](const UVoxelGraph& InGraph)
		{
			return InGraph.FindParameter(Guid);
		},
		[Guid = Guid](UVoxelGraph& InGraph, const FVoxelParameter& NewParameter)
		{
			InGraph.UpdateParameter(Guid, [&](FVoxelParameter& InParameter)
			{
				InParameter = NewParameter;
			});
		});
	KeepAlive(Wrapper);

	Wrapper->InstanceMetadataMap.Add("FilterTypes", "AllUniformsAndBufferArrays");

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Parameter", INVTEXT("Parameter"));
	Wrapper->AddChildrenTo(Category);

	KeepAlive(FVoxelParameterOverridesDetails::Create(
		DetailLayout,
		FVoxelEditorUtilities::GetObjectsBeingCustomized<UVoxelGraph>(DetailLayout),
		FVoxelEditorUtilities::MakeRefreshDelegate(this, DetailLayout),
		Guid));
}