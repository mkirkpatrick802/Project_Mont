// Copyright Voxel Plugin, Inc. All Rights Reserved.

#include "VoxelBlueprintEditorFunctionLibrary.h"

#include "VoxelActor.h"
#include "VoxelGraph.h"
#include "VoxelTransaction.h"

#include "IAssetTools.h"
#include "AssetToolsModule.h"

EVoxelSuccess UVoxelBlueprintEditorFunctionLibrary::CreateNewVoxelGraphFromVoxelActor(
	AVoxelActor* SourceActor,
	const FString AssetPathAndName,
	UVoxelGraph*& VoxelGraphAsset)
{
	if (!SourceActor)
	{
		VOXEL_MESSAGE(Error, "Actor is Null");
		return EVoxelSuccess::Failed;
	}

	if (CreateNewVoxelGraphFromGraph(SourceActor->GetGraph(), AssetPathAndName, VoxelGraphAsset) != EVoxelSuccess::Succeeded)
	{
		return EVoxelSuccess::Failed;
	}

	ApplyParametersToGraph(SourceActor, VoxelGraphAsset);

	return EVoxelSuccess::Succeeded;
}

EVoxelSuccess UVoxelBlueprintEditorFunctionLibrary::CreateNewVoxelGraphFromGraph(
	UVoxelGraph* SourceGraph,
	FString AssetPathAndName,
	UVoxelGraph*& VoxelGraphAsset)
{
	if (!SourceGraph)
	{
		VOXEL_MESSAGE(Error, "From Graph is Null");
		return EVoxelSuccess::Failed;
	}

	if (!SourceGraph->IsAsset())
	{
		VOXEL_MESSAGE(Error, "From Graph must be asset");
		return EVoxelSuccess::Failed;
	}

	AssetPathAndName = UPackageTools::SanitizePackageName(AssetPathAndName);

	IAssetTools& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

	FString NewAssetName;
	FString PackageName;
	AssetToolsModule.CreateUniqueAssetName(AssetPathAndName, "", PackageName, NewAssetName);

	UVoxelGraph* NewAsset = Cast<UVoxelGraph>(AssetToolsModule.CreateAsset(NewAssetName, FPackageName::GetLongPackagePath(PackageName), UVoxelGraph::StaticClass(), nullptr));
	if (!ensure(NewAsset))
	{
		VOXEL_MESSAGE(Error, "Failed to create new asset");
		return EVoxelSuccess::Failed;
	}

	{
		FVoxelTransaction Transaction(NewAsset);
		NewAsset->SetBaseGraph(SourceGraph);
	}

	VoxelGraphAsset = NewAsset;

	return EVoxelSuccess::Succeeded;
}

EVoxelSuccess UVoxelBlueprintEditorFunctionLibrary::ApplyParametersToGraph(
	const TScriptInterface<IVoxelParameterOverridesObjectOwner> SourceParametersObject,
	UVoxelGraph* DestinationGraph)
{
	if (!DestinationGraph)
	{
		VOXEL_MESSAGE(Error, "Graph is Null");
		return EVoxelSuccess::Failed;
	}

	if (!SourceParametersObject)
	{
		VOXEL_MESSAGE(Error, "Parameters object is Null");
		return EVoxelSuccess::Failed;
	}

	FVoxelTransaction Transaction(DestinationGraph);
	for (const auto& It : SourceParametersObject->GetParameterOverrides().PathToValueOverride)
	{
		DestinationGraph->SetParameter(It.Value.CachedName, It.Value.Value);
	}
	DestinationGraph->FixupParameterOverrides();

	return EVoxelSuccess::Succeeded;
}