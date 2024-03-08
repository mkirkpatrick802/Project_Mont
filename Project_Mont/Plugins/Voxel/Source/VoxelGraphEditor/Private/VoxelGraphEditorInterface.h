// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelTerminalGraphRuntime.h"

class FVoxelGraphEditorInterface : public IVoxelGraphEditorInterface
{
public:
	//~ Begin IVoxelGraphEditorCompiler Interface
	virtual void CompileAll() override;
	virtual void ReconstructAllNodes(UVoxelTerminalGraph& TerminalGraph) override;
	virtual bool HasNode(const UVoxelTerminalGraph& TerminalGraph, const UScriptStruct* Struct) override;
	virtual UEdGraph* CreateEdGraph(UVoxelTerminalGraph& TerminalGraph) override;
	virtual FVoxelSerializedGraph Translate(UVoxelTerminalGraph& TerminalGraph) override;
	//~ End IVoxelGraphEditorCompiler Interface
};