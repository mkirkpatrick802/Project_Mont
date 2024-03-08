// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Preview/VoxelPreviewHandler.h"
#include "VoxelQuery.h"
#include "VoxelTerminalGraphInstance.h"

TVoxelArray<TSharedRef<const FVoxelPreviewHandler>> GVoxelPreviewHandlers;

VOXEL_RUN_ON_STARTUP_GAME(RegisterVoxelPreviewHandlersCleanup)
{
	GOnVoxelModuleUnloaded_DoCleanup.AddLambda([]
	{
		GVoxelPreviewHandlers.Empty();
	});
}

TConstVoxelArrayView<TSharedRef<const FVoxelPreviewHandler>> FVoxelPreviewHandler::GetHandlers()
{
	if (GVoxelPreviewHandlers.Num() == 0)
	{
		for (const UScriptStruct* Struct : GetDerivedStructs<FVoxelPreviewHandler>())
		{
			GVoxelPreviewHandlers.Add(MakeSharedStruct<FVoxelPreviewHandler>(Struct));
		}
	}

	return GVoxelPreviewHandlers;
}

FVoxelPreviewHandler::~FVoxelPreviewHandler()
{
	if (RuntimeInfo)
	{
		// Requires manual destroy
		RuntimeInfo->Destroy();
	}
}

void FVoxelPreviewHandler::BuildStats(const FAddStat& AddStat)
{
	AddStat(
		"Position",
		"The position currently being previewed",
		MakeWeakPtrLambda(this, [this]
		{
			return CurrentPosition.ToString();
		}));
}

void FVoxelPreviewHandler::UpdateStats(const FVector2D& MousePosition)
{
	const FMatrix PixelToWorld = TerminalGraphInstance->RuntimeInfo->GetLocalToWorld().Get_NoDependency();
	CurrentPosition = PixelToWorld.TransformPosition(FVector(MousePosition.X, MousePosition.Y, 0.f));
}