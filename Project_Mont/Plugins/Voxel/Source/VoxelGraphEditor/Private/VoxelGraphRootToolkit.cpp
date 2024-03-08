// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphRootToolkit.h"
#include "VoxelGraphToolkit.h"
#include "VoxelActor.h"
#include "VoxelRuntime.h"

TArray<FVoxelToolkit::FMode> FVoxelGraphRootToolkit::GetModes() const
{
	TArray<FMode> Modes;
	{
		FMode Mode;
		Mode.Struct = FVoxelGraphPreviewToolkit::StaticStruct();
		Mode.Object = Asset;
		Mode.DisplayName = INVTEXT("Preview");
		Mode.Icon = FAppStyle::GetBrush("UMGEditor.SwitchToDesigner");
		Modes.Add(Mode);
	}
	{
		FMode Mode;
		Mode.Struct = FVoxelGraphToolkit::StaticStruct();
		Mode.Object = Asset;
		Mode.DisplayName = INVTEXT("Graph");
		Mode.Icon = FAppStyle::GetBrush("FullBlueprintEditor.SwitchToScriptingMode");
		Modes.Add(Mode);
	}
	return Modes;
}

UScriptStruct* FVoxelGraphRootToolkit::GetDefaultMode() const
{
	return FVoxelGraphToolkit::StaticStruct();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphPreviewToolkit::SetupPreview()
{
	VOXEL_FUNCTION_COUNTER();

	Super::SetupPreview();

	VoxelActor = SpawnActor<AVoxelActor>();
	if (!ensure(VoxelActor))
	{
		return;
	}

	VoxelActor->SetGraph(Asset);
	VoxelActor->DestroyRuntime();
	VoxelActor->CreateRuntime();
}

TOptional<float> FVoxelGraphPreviewToolkit::GetInitialViewDistance() const
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(VoxelActor))
	{
		return {};
	}

	const TSharedPtr<FVoxelRuntime> Runtime = VoxelActor->GetRuntime();
	if (!ensure(Runtime))
	{
		return {};
	}

	const FVoxelOptionalBox Bounds = Runtime->GetBounds();
	if (!Bounds.IsValid())
	{
		return {};
	}

	if (Bounds.GetBox().IsInfinite())
	{
		return {};
	}

	return Bounds.GetBox().Size().Length();
}