// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelExecNode_CallTerminalGraph.generated.h"

struct FVoxelGraphOutput;
struct FVoxelComputeInput;
class FVoxelGraphExecutor;
class FVoxelGraphEvaluatorRef;
using FVoxelInputGuidToEvaluatorRef = TVoxelMap<FGuid, TSharedPtr<const FVoxelGraphEvaluatorRef>>;

USTRUCT(meta = (Internal))
struct VOXELGRAPHCORE_API FVoxelExecNode_CallTerminalGraph : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	virtual void AddPins() {}
	virtual void FixupPins(
		const UVoxelGraph& Context,
		const FOnVoxelGraphChanged& OnChanged) VOXEL_PURE_VIRTUAL();

	TSharedRef<FVoxelInputGuidToEvaluatorRef> GetInputGuidToEvaluatorRef_GameThread() const;

protected:
	void FixupPinsImpl(
		const UVoxelTerminalGraph* BaseTerminalGraph,
		const FOnVoxelGraphChanged& OnChanged);
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_CallTerminalGraph : public FVoxelExecNodeRuntime
{
public:
	using FVoxelExecNodeRuntime::FVoxelExecNodeRuntime;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Tick(FVoxelRuntime& Runtime) override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FVoxelOptionalBox GetBounds() const override;
	//~ End FVoxelExecNodeRuntime Interface

protected:
	TSharedPtr<FVoxelGraphExecutor> Executor;
};