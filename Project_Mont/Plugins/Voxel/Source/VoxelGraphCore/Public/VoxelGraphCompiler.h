// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelCompilationGraph.h"
#include "VoxelTerminalGraphRuntime.h"

class VOXELGRAPHCORE_API FVoxelGraphCompiler : public Voxel::Graph::FGraph
{
public:
	using FPin = Voxel::Graph::FPin;
	using FNode = Voxel::Graph::FNode;
	using ENodeType = Voxel::Graph::ENodeType;
	using EPinDirection = Voxel::Graph::EPinDirection;

public:
	const UVoxelTerminalGraph& TerminalGraph;
	const FVoxelSerializedGraph& SerializedGraph;

	explicit FVoxelGraphCompiler(const UVoxelTerminalGraph& TerminalGraph);

	bool LoadSerializedGraph(const FOnVoxelGraphChanged& OnTranslated);

public:
	void AddPreviewNode();
	void AddDebugNodes();
	void AddRangeNodes();
	void RemoveSplitPins();
	void AddWildcardErrors();
	void AddNoDefaultErrors();
	void CheckParameters() const;
	void CheckInputs() const;
	void CheckOutputs() const;
	void AddToBuffer();
	void RemoveLocalVariables();
	void CollapseInputs();
	void ReplaceTemplates();
	void RemovePassthroughs();
	void RemoveNodesNotLinkedToQueryableNodes();
	void CheckForLoops();

public:
	void DisconnectVirtualPins();
	void RemoveNodesNotLinkedToRoot();

private:
	bool ReplaceTemplatesImpl();
	void InitializeTemplatesPassthroughNodes(FNode& Node);
	void RemoveNodesImpl(const TVoxelArray<const FNode*>& RootNodes);
};