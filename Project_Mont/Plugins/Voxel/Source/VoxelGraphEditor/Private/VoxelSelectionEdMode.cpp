// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelSelectionEdMode.h"
#include "VoxelActor.h"
#include "EditorModes.h"
#include "EditorModeManager.h"
#include "LevelEditor.h"
#include "LevelEditorViewport.h"
#include "LevelViewportClickHandlers.h"
#include "Components/InstancedStaticMeshComponent.h"

VOXEL_RUN_ON_STARTUP_EDITOR(ActivateSelectionEdMode)
{
	FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditor.OnMapChanged().AddLambda([](UWorld* World, const EMapChangeType ChangeType)
	{
		if (ChangeType == EMapChangeType::SaveMap)
		{
			return;
		}

		FVoxelUtilities::DelayedCall([]
		{
			GLevelEditorModeTools().AddDefaultMode(GetDefault<UVoxelSelectionEdMode>()->GetID());
			GLevelEditorModeTools().ActivateDefaultMode();
		});
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelSelectionEdMode::UVoxelSelectionEdMode()
{
	Info = FEditorModeInfo(
		"VoxelSelectionEdMode",
		INVTEXT("VoxelSelectionEdMode"),
		FSlateIcon(),
		false,
		MAX_int32
	);

	SettingsClass = UVoxelSelectionEdModeSettings::StaticClass();
}

bool UVoxelSelectionEdMode::HandleClick(FEditorViewportClient* ViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	if (Click.GetKey() != EKeys::LeftMouseButton)
	{
		return false;
	}

	if (Click.GetEvent() != IE_Released &&
		Click.GetEvent() != IE_DoubleClick)
	{
		return false;
	}

	const HActor* ActorProxy = HitProxyCast<HActor>(HitProxy);
	if (!ActorProxy ||
		!ActorProxy->Actor)
	{
		return false;
	}

	if (!ActorProxy->Actor->IsA<AVoxelActor>())
	{
		return false;
	}

	if (ActorProxy->PrimComponent &&
		ActorProxy->PrimComponent->IsA<UInstancedStaticMeshComponent>())
	{
		return false;
	}

	FVector Start;
	FVector End;
	if (!ensure(FVoxelEditorUtilities::GetRayInfo(ViewportClient, Start, End)))
	{
		return false;
	}

	FHitResult HitResult;
	if (!GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_EngineTraceChannel6))
	{
		return false;
	}

	AActor* Actor = HitResult.GetActor();
	if (!ensure(Actor) ||
		!ensure(ViewportClient->IsLevelEditorClient()))
	{
		return false;
	}

	if (Click.GetEvent() == IE_DoubleClick)
	{
		FCollisionQueryParams Params = FCollisionQueryParams::DefaultQueryParam;
		Params.AddIgnoredActor(Actor);

		FHitResult FurtherHitResult;
		if (GetWorld()->LineTraceSingleByChannel(FurtherHitResult, HitResult.Location, End, ECC_EngineTraceChannel6, Params))
		{
			if (AActor* FurtherActor = FurtherHitResult.GetActor())
			{
				Actor = FurtherActor;
			}
		}
	}

	if (AActor* RootSelection = Actor->GetRootSelectionParent())
	{
		Actor = RootSelection;
	}

	LevelViewportClickHandlers::ClickActor(
		static_cast<FLevelEditorViewportClient*>(ViewportClient),
		Actor,
		Click,
		true);

	return true;
}