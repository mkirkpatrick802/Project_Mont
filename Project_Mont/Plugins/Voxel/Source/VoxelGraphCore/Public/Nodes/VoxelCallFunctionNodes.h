// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Nodes/VoxelExecNode_CallTerminalGraph.h"
#include "VoxelCallFunctionNodes.generated.h"

class FVoxelGraphExecutor;
class UVoxelFunctionLibraryAsset;

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelExecNode_CallFunction : public FVoxelExecNode_CallTerminalGraph
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	TObjectPtr<UVoxelGraph> ContextOverride;

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

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_CallFunction : public TVoxelExecNodeRuntime<FVoxelExecNode_CallFunction, FVoxelExecNodeRuntime_CallTerminalGraph>
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
struct VOXELGRAPHCORE_API FVoxelExecNode_CallExternalFunction : public FVoxelExecNode_CallTerminalGraph
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	UPROPERTY()
	TObjectPtr<UVoxelFunctionLibraryAsset> FunctionLibrary;

	UPROPERTY()
	FGuid Guid;

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

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_CallExternalFunction : public TVoxelExecNodeRuntime<FVoxelExecNode_CallExternalFunction, FVoxelExecNodeRuntime_CallTerminalGraph>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	//~ End FVoxelExecNodeRuntime Interface
};