// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFutureValueState.h"
#include "VoxelTaskGroup.h"

FVoxelFutureValueStateImpl::~FVoxelFutureValueStateImpl()
{
	if (!UObjectInitialized())
	{
		// Global exit, nothing to be done
		return;
	}

	if (!IsComplete())
	{
		SetValue(FVoxelRuntimePinValue(Type));
	}

	checkVoxelSlow(IsComplete());
	checkVoxelSlow(DependentTasks.Num() == 0);
	checkVoxelSlow(LinkedStates.Num() == 0);
}

void FVoxelFutureValueStateImpl::SetValue(const FVoxelRuntimePinValue& NewValue)
{
	checkVoxelSlow(!IsComplete());
	checkVoxelSlow(!Value.IsValid());

	checkVoxelSlow(NewValue.IsValid());
	checkVoxelSlow(NewValue.CanBeCastedTo(Type));
	checkVoxelSlow(NewValue.IsValidValue_Slow());

	Value = NewValue;

	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		checkVoxelSlow(!IsComplete());
		bIsComplete.Set(true);
	}

	for (const FDependentTask& DependentTask : DependentTasks)
	{
		const SharedPointerInternals::FSharedReferencer<ESPMode::ThreadSafe> SharedReferencer = DependentTask.GroupWeakReferencer;
		if (!SharedReferencer.IsValid())
		{
			continue;
		}

		FVoxelTask& Task = *DependentTask.Task;
		checkVoxelSlow(Task.Group.AsWeak().IsValid());
		ON_SCOPE_EXIT
		{
			checkVoxelSlow(Task.Group.AsWeak().IsValid());
		};

		const int32 NumDependenciesLeft = Task.NumDependencies.Decrement_ReturnNew();
		checkVoxelSlow(NumDependenciesLeft >= 0);

		if (NumDependenciesLeft > 0)
		{
			continue;
		}

		TVoxelUniquePtr<FVoxelTask> TaskPtr = DependentTask.Task->Group.RemovePendingTask(*DependentTask.Task);
		Task.Group.QueueTask(MoveTemp(TaskPtr));
	}
	DependentTasks.Empty();

	for (const TSharedPtr<FVoxelFutureValueStateImpl>& State : LinkedStates)
	{
		State->SetValue(Value);
	}
	LinkedStates.Empty();
}