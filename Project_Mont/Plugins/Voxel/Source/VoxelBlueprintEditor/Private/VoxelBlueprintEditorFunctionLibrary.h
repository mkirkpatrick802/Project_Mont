// Copyright Voxel Plugin, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VoxelParameterOverridesOwner.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelBlueprintEditorFunctionLibrary.generated.h"

class UVoxelGraph;
class AVoxelActor;

UCLASS()
class VOXELBLUEPRINTEDITOR_API UVoxelBlueprintEditorFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Creates a Voxel Graph asset from specified Voxel Actor,
	// will use its Graph as parent and copy all parameter values
	// @param SourceActor Voxel Actor to copy graph and parameters from
	UFUNCTION(BlueprintCallable, Category = "Voxel|Editor", meta = (ExpandEnumAsExecs = ReturnValue))
	static EVoxelSuccess CreateNewVoxelGraphFromVoxelActor(
		AVoxelActor* SourceActor,
		FString AssetPathAndName,
		UVoxelGraph*& VoxelGraphAsset);

	// Creates a Voxel Graph asset with SourceGraph as its parent
	// @param SourceGraph Voxel Graph Asset to use as parent
	UFUNCTION(BlueprintCallable, Category = "Voxel|Editor", meta = (ExpandEnumAsExecs = ReturnValue))
	static EVoxelSuccess CreateNewVoxelGraphFromGraph(
		UVoxelGraph* SourceGraph,
		FString AssetPathAndName,
		UVoxelGraph*& VoxelGraphAsset);

	// Copies and applies all parameter values from SourceParametersObject to DestinationGraph
	// @param SourceParametersObject Parameters Object (Voxel Graph / Voxel Actor) to copy values from
	// @param DestinationGraph Graph asset to apply parameter values
	UFUNCTION(BlueprintCallable, Category = "Voxel|Editor", meta = (ExpandEnumAsExecs = ReturnValue))
	static EVoxelSuccess ApplyParametersToGraph(
		TScriptInterface<IVoxelParameterOverridesObjectOwner> SourceParametersObject,
		UVoxelGraph* DestinationGraph);
};