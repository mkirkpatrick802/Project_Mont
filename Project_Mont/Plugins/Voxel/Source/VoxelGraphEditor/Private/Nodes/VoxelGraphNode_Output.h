// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphNode.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphNode_Output.generated.h"

UCLASS()
class UVoxelGraphNode_Output : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FVoxelGraphOutput CachedOutput;

	const FVoxelGraphOutput* GetOutput() const;
	FVoxelGraphOutput GetOutputSafe() const;

	//~ Begin UVoxelGraphNode Interface
	virtual bool IsVariable() const override { return true; }
	virtual void AllocateDefaultPins() override;

	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;

	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetSearchTerms() const override;
	//~ End UVoxelGraphNode Interface

private:
	FSharedVoidPtr OnOutputChangedPtr;
};