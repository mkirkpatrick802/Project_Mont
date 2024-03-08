﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Sculpt/VoxelExecNode_EditSculptSurface.h"

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_EditSculptSurface::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_EditSculptSurface>(SharedThis);
}

void FVoxelExecNodeRuntime_EditSculptSurface::Create()
{
	const TSharedPtr<const FVoxelRuntimeParameter_EditSculptSurface> Parameter = FindParameter<FVoxelRuntimeParameter_EditSculptSurface>();
	if (!Parameter)
	{
		return;
	}

	VOXEL_SCOPE_LOCK(Parameter->CriticalSection);

	if (const TSharedPtr<FVoxelExecNodeRuntime_EditSculptSurface> Runtime = Parameter->WeakRuntime.Pin())
	{
		VOXEL_MESSAGE(Error, "Multiple Edit Sculpt Surface nodes: {0}, {1}", this, Runtime.Get());
		return;
	}

	Parameter->WeakRuntime = SharedThis(this);
}

void FVoxelExecNodeRuntime_EditSculptSurface::Destroy()
{
	Super::Destroy();

	const TSharedPtr<const FVoxelRuntimeParameter_EditSculptSurface> Parameter = FindParameter<FVoxelRuntimeParameter_EditSculptSurface>();
	if (!Parameter)
	{
		return;
	}

	VOXEL_SCOPE_LOCK(Parameter->CriticalSection);

	if (GetWeakPtrObject_Unsafe(Parameter->WeakRuntime) == this)
	{
		Parameter->WeakRuntime = {};
	}
}