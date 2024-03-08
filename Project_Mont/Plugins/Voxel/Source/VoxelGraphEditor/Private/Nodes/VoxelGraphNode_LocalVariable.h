// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelTerminalGraph.h"
#include "Nodes/VoxelGraphNode.h"
#include "VoxelGraphNode_LocalVariable.generated.h"

UCLASS(Abstract)
class UVoxelGraphNode_LocalVariable : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FVoxelGraphLocalVariable CachedLocalVariable;

	const FVoxelGraphLocalVariable* GetLocalVariable() const;
	FVoxelGraphLocalVariable GetLocalVariableSafe() const;

	//~ Begin UVoxelGraphNode Interface
	virtual bool IsVariable() const override { return true; }
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetSearchTerms() const override;
	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;
	//~ End UVoxelGraphNode Interface

private:
	FSharedVoidPtr OnLocalVariableChangedPtr;
};

UCLASS()
class UVoxelGraphNode_LocalVariableDeclaration : public UVoxelGraphNode_LocalVariable
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraphLocalVariableNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End UVoxelGraphLocalVariableNode Interface
};

UCLASS()
class UVoxelGraphNode_LocalVariableUsage : public UVoxelGraphNode_LocalVariable
{
	GENERATED_BODY()

public:
	//~ Begin UVoxelGraphLocalVariableNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool CanJumpToDefinition() const override { return true; }
	virtual void JumpToDefinition() const override;
	//~ End UVoxelGraphLocalVariableNode Interface

	UVoxelGraphNode_LocalVariableDeclaration* FindDeclaration() const;
};