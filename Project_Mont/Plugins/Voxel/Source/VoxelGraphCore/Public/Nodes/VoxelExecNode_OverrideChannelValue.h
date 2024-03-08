// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBounds.h"
#include "VoxelExecNode.h"
#include "Channel/VoxelChannelName.h"
#include "VoxelExecNode_OverrideChannelValue.generated.h"

class FVoxelBrushRef;

USTRUCT(Category = "Channel|Advanced")
struct VOXELGRAPHCORE_API FVoxelExecNode_OverrideChannelValue : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelChannelName, Channel, nullptr, ConstantPin);
	VOXEL_INPUT_PIN(int32, Priority, nullptr, ConstantPin);
	VOXEL_INPUT_PIN(FVoxelBounds, Bounds, nullptr, ConstantPin);
	VOXEL_INPUT_PIN(FVoxelWildcard, Value, nullptr, VirtualPin);

	FVoxelExecNode_OverrideChannelValue();

	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;

#if WITH_EDITOR
	class FDefinition : public Super::FDefinition
	{
	public:
		GENERATED_VOXEL_NODE_DEFINITION_BODY(FVoxelExecNode_OverrideChannelValue);

		virtual void Initialize(UEdGraphNode& EdGraphNode) override;
		virtual bool OnPinDefaultValueChanged(FName PinName, const FVoxelPinValue& NewDefaultValue) override;
	};
#endif
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_OverrideChannelValue : public TVoxelExecNodeRuntime<FVoxelExecNode_OverrideChannelValue>
{
public:
	using Super::Super;

	virtual void Create() override;
	virtual void Destroy() override;

private:
	TSharedPtr<FVoxelBrushRef> BrushRef;
};