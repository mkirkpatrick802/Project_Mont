// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelExecNode.h"
#include "VoxelRuntime.h"
#include "VoxelTerminalGraphInstance.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelExecNodeRuntime);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelExecNodeRuntime> FVoxelExecNode::CreateSharedExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	TVoxelUniquePtr<FVoxelExecNodeRuntime> ExecRuntime = CreateExecRuntime(SharedThis);
	if (!ExecRuntime)
	{
		return nullptr;
	}

	return TSharedPtr<FVoxelExecNodeRuntime>(ExecRuntime.Release(), [](FVoxelExecNodeRuntime* Runtime)
	{
		if (!Runtime)
		{
			return;
		}

		RunOnGameThread([Runtime]
		{
			if (Runtime->bIsCreated)
			{
				Runtime->CallDestroy();
			}
			FVoxelMemory::Delete(Runtime);
		});
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelExecNodeRuntime::~FVoxelExecNodeRuntime()
{
	ensure(!bIsCreated || bIsDestroyed);
}

TSharedPtr<FVoxelRuntime> FVoxelExecNodeRuntime::GetRuntime() const
{
	const TSharedPtr<FVoxelRuntime> Runtime = PrivateTerminalGraphInstance->RuntimeInfo->GetRuntime();
	ensure(Runtime || IsDestroyed());
	return Runtime;
}

USceneComponent* FVoxelExecNodeRuntime::GetRootComponent() const
{
	ensure(IsInGameThread());
	USceneComponent* RootComponent = PrivateTerminalGraphInstance->RuntimeInfo->GetRootComponent();
	ensure(RootComponent);
	return RootComponent;
}

const TSharedRef<const FVoxelRuntimeInfo>& FVoxelExecNodeRuntime::GetRuntimeInfo() const
{
	return PrivateTerminalGraphInstance->RuntimeInfo;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime::CallCreate(
	const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
	TVoxelMap<FName, TVoxelArray<FVoxelRuntimePinValue>>&& ConstantValues)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(!bIsCreated);
	bIsCreated = true;

	PrivateTerminalGraphInstance = TerminalGraphInstance;
	PrivateConstantValues = MoveTemp(ConstantValues);

	const FVoxelQueryScope Scope(nullptr, &GetTerminalGraphInstance().Get());

	PreCreate();
	Create();
}

void FVoxelExecNodeRuntime::CallDestroy()
{
	VOXEL_FUNCTION_COUNTER();

	ensure(bIsCreated);
	ensure(!bIsDestroyed);
	bIsDestroyed = true;

	const FVoxelQueryScope Scope(nullptr, &GetTerminalGraphInstance().Get());

	Destroy();
}