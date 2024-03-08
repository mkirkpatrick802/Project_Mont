// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphSearchManager.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

FVoxelGraphSearchManager* GVoxelGraphSearchManager = new FVoxelGraphSearchManager();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphSearchManager::Initialize()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FGlobalTabmanager> TabManager = FGlobalTabmanager::Get();

	TabManager->RegisterNomadTabSpawner(GlobalSearchTabId, MakeLambdaDelegate([=](const FSpawnTabArgs& SpawnTabArgs)
	{
		return
			SNew(SDockTab)
			.TabRole(NomadTab)
			.Label(INVTEXT("Find in Voxel Graphs"))
			.ToolTipText(INVTEXT("Search for a string in all Voxel Graph assets"))
			[
				SNew(SVoxelGraphSearch)
				.IsGlobalSearch(true)
			];
	}))
	.SetDisplayName(INVTEXT("Find in Voxel Graphs"))
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "BlueprintEditor.FindInBlueprints.MenuIcon"))
	.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<SVoxelGraphSearch> FVoxelGraphSearchManager::OpenGlobalSearch() const
{
	const TSharedPtr<SDockTab> NewTab = FGlobalTabmanager::Get()->TryInvokeTab(GlobalSearchTabId);
	if (!ensure(NewTab))
	{
		return nullptr;
	}
	return StaticCastSharedRef<SVoxelGraphSearch>(NewTab->GetContent());
}