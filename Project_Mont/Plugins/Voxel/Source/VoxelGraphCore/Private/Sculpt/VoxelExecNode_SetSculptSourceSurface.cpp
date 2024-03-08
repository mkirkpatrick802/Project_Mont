// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/VoxelExecNode_SetSculptSourceSurface.h"
#include "Sculpt/VoxelSculptStorage.h"
#include "Sculpt/VoxelSculptStorageData.h"
#include "VoxelDependency.h"

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_SetSculptSourceSurface::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_SetSculptSourceSurface>(SharedThis);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_SetSculptSourceSurface::Create()
{
	VOXEL_FUNCTION_COUNTER();

	if (IsPreviewScene())
	{
		return;
	}

	const TSharedPtr<const FVoxelRuntimeParameter_SculptStorage> Parameter = FindParameter<FVoxelRuntimeParameter_SculptStorage>();
	if (!ensure(Parameter))
	{
		return;
	}

	{
		VOXEL_SCOPE_LOCK(Parameter->CriticalSection);
		Parameter->VoxelSize = GetConstantPin(Node.VoxelSizePin);
		Parameter->Compute_RequiresLock = GetNodeRuntime().GetCompute(Node.SurfacePin, GetTerminalGraphInstance());
	}

	Parameter->Data->Dependency->Invalidate();
}

void FVoxelExecNodeRuntime_SetSculptSourceSurface::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<const FVoxelRuntimeParameter_SculptStorage> Parameter = FindParameter<FVoxelRuntimeParameter_SculptStorage>();
	if (!Parameter)
	{
		return;
	}

	{
		VOXEL_SCOPE_LOCK(Parameter->CriticalSection);
		Parameter->VoxelSize = 0.f;
		Parameter->Compute_RequiresLock = nullptr;
	}

	Parameter->Data->Dependency->Invalidate();
}