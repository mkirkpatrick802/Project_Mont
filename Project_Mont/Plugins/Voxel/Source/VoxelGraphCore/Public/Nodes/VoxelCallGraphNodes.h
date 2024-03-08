// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/VoxelExecNode_CallTerminalGraph.h"
#include "VoxelCallGraphNodes.generated.h"

class FVoxelGraphExecutor;

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelExecNode_CallGraph : public FVoxelExecNode_CallTerminalGraph
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	UPROPERTY()
	TObjectPtr<UVoxelGraph> Graph;

	//~ Begin FVoxelNode_CallTerminalGraph Interface
	virtual void FixupPins(
		const UVoxelGraph& Context,
		const FOnVoxelGraphChanged& OnChanged) override;
	//~ End FVoxelNode_CallTerminalGraph Interface

	//~ Begin FVoxelExecNode Interface
	virtual FVoxelComputeValue CompileCompute(FName PinName) const override;
	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
	//~ End FVoxelExecNode Interface
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_CallGraph : public TVoxelExecNodeRuntime<FVoxelExecNode_CallGraph, FVoxelExecNodeRuntime_CallTerminalGraph>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	//~ End FVoxelExecNodeRuntime Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelExecNode_CallGraphParameter : public FVoxelExecNode_CallTerminalGraph
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	UPROPERTY()
	TObjectPtr<UVoxelGraph> BaseGraph;

	//~ Begin FVoxelNode_CallTerminalGraph Interface
	virtual void AddPins() override;
	virtual void FixupPins(
		const UVoxelGraph& Context,
		const FOnVoxelGraphChanged& OnChanged) override;
	//~ End FVoxelNode_CallTerminalGraph Interface

	//~ Begin FVoxelExecNode Interface
	virtual FVoxelComputeValue CompileCompute(FName PinName) const override;
	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
	//~ End FVoxelExecNode Interface
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_CallGraphParameter : public TVoxelExecNodeRuntime<FVoxelExecNode_CallGraphParameter, FVoxelExecNodeRuntime_CallTerminalGraph>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	//~ End FVoxelExecNodeRuntime Interface
};