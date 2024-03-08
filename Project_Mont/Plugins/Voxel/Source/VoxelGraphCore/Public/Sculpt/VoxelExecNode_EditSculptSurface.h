// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelBounds.h"
#include "VoxelSurface.h"
#include "VoxelExecNode.h"
#include "VoxelExecNode_EditSculptSurface.generated.h"

// Used in sculpt tools
USTRUCT()
struct VOXELGRAPHCORE_API FVoxelExecNode_EditSculptSurface : public FVoxelExecNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	VOXEL_INPUT_PIN(FVoxelSurface, NewSurface, nullptr, VirtualPin);
	VOXEL_INPUT_PIN(FVoxelBounds, Bounds, nullptr, VirtualPin);

	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const override;
};

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime_EditSculptSurface : public TVoxelExecNodeRuntime<FVoxelExecNode_EditSculptSurface>
{
public:
	using Super::Super;

	//~ Begin FVoxelExecNodeRuntime Interface
	virtual void Create() override;
	virtual void Destroy() override;
	//~ End FVoxelExecNodeRuntime Interface
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelRuntimeParameter_EditSculptSurface : public FVoxelRuntimeParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	mutable FVoxelCriticalSection CriticalSection;
	mutable TWeakPtr<FVoxelExecNodeRuntime_EditSculptSurface> WeakRuntime;
};