// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEditorMinimal.h"
#include "LevelEditor.h"

class FVoxelRefreshAllGraphCommands : public TVoxelCommands<FVoxelRefreshAllGraphCommands>
{
public:
	TSharedPtr<FUICommandInfo> RefreshAll;

	virtual void RegisterCommands() override;
};

DEFINE_VOXEL_COMMANDS(FVoxelRefreshAllGraphCommands);

void FVoxelRefreshAllGraphCommands::RegisterCommands()
{
	VOXEL_UI_COMMAND(RefreshAll, "Refresh", "Refresh all voxel graphs", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F5));
}

VOXEL_RUN_ON_STARTUP_EDITOR(RegisterVoxelEditorCommands)
{
	FVoxelRefreshAllGraphCommands::Register();

	const FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	FUICommandList& Actions = *LevelEditorModule.GetGlobalLevelEditorActions();

	Actions.MapAction(FVoxelRefreshAllGraphCommands::Get().RefreshAll, MakeLambdaDelegate([]
	{
		GEngine->Exec(nullptr, TEXT("voxel.RefreshAll"));
	}));
}