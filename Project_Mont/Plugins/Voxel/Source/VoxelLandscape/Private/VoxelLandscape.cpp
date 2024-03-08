// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscape.h"
#include "VoxelLandscapeRuntime.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_EDITOR(RegisterOnApplyObjectToActor)
{
	FEditorDelegates::OnApplyObjectToActor.AddLambda([](UObject* Object, AActor* Actor)
	{
		AVoxelLandscape* Landscape = Cast<AVoxelLandscape>(Actor);
		if (!Landscape)
		{
			return;
		}

		if (UMaterialInterface* Material = Cast<UMaterialInterface>(Object))
		{
			Landscape->Material = Material;
			return;
		}
	});
}
#endif

TSharedPtr<IVoxelActorRuntime> AVoxelLandscape::CreateNewRuntime()
{
	const TSharedRef<FVoxelLandscapeRuntime> Runtime = MakeVoxelShared<FVoxelLandscapeRuntime>(*this);
	Runtime->Initialize();
	return Runtime;
}