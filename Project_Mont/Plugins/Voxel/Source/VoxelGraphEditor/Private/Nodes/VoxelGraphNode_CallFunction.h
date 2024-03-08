// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Nodes/VoxelGraphNode_CallTerminalGraph.h"
#include "VoxelGraphNode_CallFunction.generated.h"

class UVoxelGraph;
class UVoxelTerminalGraph;
class UVoxelFunctionLibraryAsset;

UCLASS()
class UVoxelGraphNode_CallFunction : public UVoxelGraphNode_CallTerminalGraph
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	bool bCallParent = false;

	UPROPERTY()
	FString CachedName;

	//~ Begin UVoxelGraphNode_CallTerminalGraph Interface
	virtual const UVoxelTerminalGraph* GetBaseTerminalGraph() const override;
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;
	//~ End UVoxelGraphNode_CallTerminalGraph Interface

private:
	FSharedVoidPtr OnFunctionMetaDataChangedPtr;
};

UCLASS()
class UVoxelGraphNode_CallExternalFunction : public UVoxelGraphNode_CallTerminalGraph
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UVoxelFunctionLibraryAsset> FunctionLibrary;

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FString CachedName;

	//~ Begin UVoxelGraphNode_CallTerminalGraph Interface
	virtual const UVoxelTerminalGraph* GetBaseTerminalGraph() const override;
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UVoxelGraphNode_CallTerminalGraph Interface

private:
	FSharedVoidPtr OnFunctionMetaDataChangedPtr;
};