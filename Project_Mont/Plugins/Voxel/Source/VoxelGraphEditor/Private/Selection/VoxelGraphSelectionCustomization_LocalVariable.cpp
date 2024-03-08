// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Selection/VoxelGraphSelectionCustomization_LocalVariable.h"
#include "VoxelTerminalGraph.h"

void FVoxelGraphSelectionCustomization_LocalVariable::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	DetailLayout.HideCategory("Config");
	DetailLayout.HideCategory("Function");

	if (!Guid.IsValid())
	{
		return;
	}

	const TSharedRef<FVoxelStructDetailsWrapper> Wrapper = FVoxelStructDetailsWrapper::Make<UVoxelTerminalGraph, FVoxelGraphLocalVariable>(
		DetailLayout,
		[Guid = Guid](const UVoxelTerminalGraph& InTerminalGraph)
		{
			return InTerminalGraph.FindLocalVariable(Guid);
		},
		[Guid = Guid](UVoxelTerminalGraph& InTerminalGraph, const FVoxelGraphLocalVariable& NewLocalVariable)
		{
			InTerminalGraph.UpdateLocalVariable(Guid, [&](FVoxelGraphLocalVariable& InLocalVariable)
			{
				InLocalVariable = NewLocalVariable;
			});
		});
	KeepAlive(Wrapper);

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Local Variable", INVTEXT("Local Variable"));
	Wrapper->AddChildrenTo(Category);
}