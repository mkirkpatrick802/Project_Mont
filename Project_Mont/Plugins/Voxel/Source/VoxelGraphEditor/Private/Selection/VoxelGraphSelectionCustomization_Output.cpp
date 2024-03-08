// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Selection/VoxelGraphSelectionCustomization_Output.h"
#include "VoxelTerminalGraph.h"

void FVoxelGraphSelectionCustomization_Output::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	DetailLayout.HideCategory("Config");
	DetailLayout.HideCategory("Function");

	if (!Guid.IsValid())
	{
		return;
	}

	const TSharedRef<FVoxelStructDetailsWrapper> Wrapper = FVoxelStructDetailsWrapper::Make<UVoxelTerminalGraph, FVoxelGraphOutput>(
		DetailLayout,
		[Guid = Guid](const UVoxelTerminalGraph& InTerminalGraph)
		{
			return InTerminalGraph.FindOutput(Guid);
		},
		[Guid = Guid](UVoxelTerminalGraph& InTerminalGraph, const FVoxelGraphOutput& NewOutput)
		{
			InTerminalGraph.UpdateOutput(Guid, [&](FVoxelGraphOutput& InOutput)
			{
				InOutput = NewOutput;
			});
		});
	KeepAlive(Wrapper);

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Output", INVTEXT("Output"));
	Wrapper->AddChildrenTo(Category);
}