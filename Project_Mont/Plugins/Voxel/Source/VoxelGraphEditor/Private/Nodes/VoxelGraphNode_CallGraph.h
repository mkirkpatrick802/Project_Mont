// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "Nodes/VoxelGraphNode_CallTerminalGraph.h"
#include "VoxelGraphNode_CallGraph.generated.h"

class UVoxelGraph;
class UVoxelTerminalGraph;

UCLASS()
class UVoxelGraphNode_CallGraph : public UVoxelGraphNode_CallTerminalGraph
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelGraph> Graph;

	//~ Begin UVoxelGraphNode_CallTerminalGraph Interface
	virtual const UVoxelTerminalGraph* GetBaseTerminalGraph() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UVoxelGraphNode_CallTerminalGraph Interface
};

UCLASS()
class UVoxelGraphNode_CallGraphParameter : public UVoxelGraphNode_CallTerminalGraph
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Config")
	TObjectPtr<UVoxelGraph> BaseGraph;

	//~ Begin UVoxelGraphNode_CallTerminalGraph Interface
	virtual void AddPins() override;
	virtual const UVoxelTerminalGraph* GetBaseTerminalGraph() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UVoxelGraphNode_CallTerminalGraph Interface
};