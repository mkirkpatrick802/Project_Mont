// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelExecNode.h"
#include "VoxelInstancedMeshComponentSettings.h"
#include "Point/VoxelChunkedPointSet.h"
#include "VoxelExecNode_OutputPointMesh.generated.h"

class FVoxelInvokerView;
class FVoxelRenderMeshChunk;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelExecNode_OutputPointMesh : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelChunkedPointSet, ChunkedPoints, nullptr, ConstantPin);
	VOXEL_INPUT_PIN(float, RenderDistance, 10000.f, ConstantPin);
	VOXEL_INPUT_PIN(float, MinRenderDistance, 0.f, ConstantPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(float, FadeDistance, 1000.f, ConstantPin, AdvancedDisplay);
	VOXEL_INPUT_PIN(FVoxelInstancedMeshComponentSettings, FoliageSettings, nullptr, ConstantPin, AdvancedDisplay);
	// Priority offset, added to the task distance from camera
	// Closest tasks are computed first, so set this to a very low value (eg, -1000000) if you want it to be computed first
	VOXEL_INPUT_PIN(double, PriorityOffset, 0, ConstantPin, AdvancedDisplay);

	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_OutputPointMesh : public TVoxelExecNodeRuntime<FVoxelExecNode_OutputPointMesh>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	virtual void Destroy() override;
	virtual FVoxelOptionalBox GetBounds() const override;
	//~ End FVoxelExecNodeRuntime Interface

private:
	TSharedPtr<FVoxelInvokerView> InvokerView;

	mutable FVoxelCriticalSection CriticalSection;
	TVoxelMap<FIntVector, TSharedPtr<FVoxelRenderMeshChunk>> Chunks_RequiresLock;
};