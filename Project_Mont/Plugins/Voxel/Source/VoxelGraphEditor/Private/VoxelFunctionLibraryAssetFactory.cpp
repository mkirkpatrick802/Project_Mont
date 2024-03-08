// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFunctionLibraryAssetFactory.h"
#include "VoxelTerminalGraph.h"
#include "VoxelFunctionLibraryAsset.h"

UVoxelFunctionLibraryAssetFactory::UVoxelFunctionLibraryAssetFactory()
{
	SupportedClass = UVoxelFunctionLibraryAsset::StaticClass();

	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UVoxelFunctionLibraryAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, const FName Name, const EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UVoxelFunctionLibraryAsset* FunctionLibrary = NewObject<UVoxelFunctionLibraryAsset>(InParent, Class, Name, Flags);
	if (!FunctionLibrary)
	{
		return nullptr;
	}

	UVoxelTerminalGraph& TerminalGraph = FunctionLibrary->GetGraph().AddTerminalGraph(FGuid::NewGuid());
	TerminalGraph.UpdateMetadata([&](FVoxelGraphMetadata& Metadata)
	{
		Metadata.DisplayName = Name.ToString();
	});

	return FunctionLibrary;
}