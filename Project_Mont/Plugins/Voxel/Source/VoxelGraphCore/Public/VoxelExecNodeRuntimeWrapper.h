// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"

class VOXELGRAPHCORE_API FVoxelExecNodeRuntimeWrapper : public TSharedFromThis<FVoxelExecNodeRuntimeWrapper>
{
public:
	const TSharedRef<FVoxelExecNode> Node;

	explicit FVoxelExecNodeRuntimeWrapper(const TSharedRef<FVoxelExecNode>& Node)
		: Node(Node)
	{
	}

	void Initialize(const TSharedRef<FVoxelTerminalGraphInstance>& NewTerminalGraphInstance);

	void Tick(FVoxelRuntime& Runtime) const;
	void AddReferencedObjects(FReferenceCollector& Collector) const;
	FVoxelOptionalBox GetBounds() const;

private:
	TSharedPtr<FVoxelTerminalGraphInstance> TerminalGraphInstance;
	TVoxelDynamicValue<bool> EnableNodeValue;

	struct FConstantValue : TSharedFromThis<FConstantValue>
	{
		FVoxelDynamicValue DynamicValue;
		FVoxelRuntimePinValue Value;
	};
	TMap<FName, TVoxelArray<TSharedPtr<FConstantValue>>> ConstantPins_GameThread;

	TSharedPtr<FVoxelExecNodeRuntime> NodeRuntime_GameThread;

	void ComputeConstantPins();
	void OnConstantValueUpdated();
};