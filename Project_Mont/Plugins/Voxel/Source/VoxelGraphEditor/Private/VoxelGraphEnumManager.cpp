// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.h"
#include "VoxelGraph.h"
#include "VoxelTerminalGraph.h"
#include "Kismet2/EnumEditorUtils.h"

class FVoxelGraphEnumManager : public FEnumEditorUtils::INotifyOnEnumChanged
{
	virtual void PreChange(const UUserDefinedEnum* Changed, FEnumEditorUtils::EEnumEditorChangeInfo ChangedType) override
	{
	}

	virtual void PostChange(const UUserDefinedEnum* Enum, FEnumEditorUtils::EEnumEditorChangeInfo ChangedType) override
	{
		ForEachObjectOfClass<UVoxelGraph>([&](UVoxelGraph& Graph)
		{
			Graph.Fixup();
		}, true, RF_ClassDefaultObject | RF_Transient);

		ForEachObjectOfClass<UVoxelTerminalGraph>([&](UVoxelTerminalGraph& TerminalGraph)
		{
			TerminalGraph.Fixup();
		}, true, RF_ClassDefaultObject | RF_Transient);

		// TODO Reconstruct any node with a pin of type Enum
	}
};

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterEnumManager)
{
	new FVoxelGraphEnumManager();
}