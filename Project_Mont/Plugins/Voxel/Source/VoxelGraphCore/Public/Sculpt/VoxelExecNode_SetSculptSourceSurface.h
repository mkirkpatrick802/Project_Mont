// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelSurface.h"
#include "VoxelExecNode.h"
#include "VoxelExecNode_SetSculptSourceSurface.generated.h"

// Set the surface that the sculpt data should default to
// ie, the surface that should be used if no edits have been made
USTRUCT()
struct VOXELGRAPHCORE_API FVoxelExecNode_SetSculptSourceSurface : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr, VirtualPin);
	VOXEL_INPUT_PIN(int32, VoxelSize, 100, ConstantPin);

	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_SetSculptSourceSurface : public TVoxelExecNodeRuntime<FVoxelExecNode_SetSculptSourceSurface>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	virtual void Destroy() override;
	//~ End FVoxelExecNodeRuntime Interface
};