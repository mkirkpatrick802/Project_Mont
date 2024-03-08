// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTaskGroup.h"
#include "VoxelFutureValue.h"

class VOXELGRAPHCORE_API FVoxelFutureValueStateImpl
	: public FVoxelFutureValueState
	, public TSharedFromThis<FVoxelFutureValueStateImpl>
{
public:
	FORCEINLINE FVoxelFutureValueStateImpl(
		const FVoxelPinType& Type,
		const FName DebugName)
		: FVoxelFutureValueState(Type)
		, DebugName(DebugName)
	{
	}
	~FVoxelFutureValueStateImpl();
	UE_NONCOPYABLE(FVoxelFutureValueStateImpl);

	FORCEINLINE bool TryAddDependentTask(FVoxelTask& Task)
	{
		VOXEL_SCOPE_LOCK(CriticalSection);

		if (IsComplete())
		{
			return false;
		}

		DependentTasks.Add(FDependentTask
		{
			GetWeakPtrWeakReferencer(GetSharedFromThisWeakPtr(Task.Group)),
			&Task
		});
		return true;
	}
	FORCEINLINE bool TryAddLinkedState(const TSharedRef<FVoxelFutureValueStateImpl>& State)
	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		checkVoxelSlow(Type.CanBeCastedTo(State->Type));

		if (IsComplete())
		{
			return false;
		}

		LinkedStates.Add(State);
		return true;
	}
	void SetValue(const FVoxelRuntimePinValue& NewValue);

private:
	struct FDependentTask
	{
		SharedPointerInternals::FWeakReferencer<ESPMode::ThreadSafe> GroupWeakReferencer;
		FVoxelTask* Task = nullptr;
	};

	const FMinimalName DebugName;
	FVoxelCriticalSection_NoPadding CriticalSection;
	TVoxelInlineArray<FDependentTask, 8> DependentTasks;
	TVoxelInlineArray<TSharedPtr<FVoxelFutureValueStateImpl>, 8> LinkedStates;
};

template<typename T>
class TVoxelFutureValueState : public FVoxelFutureValueStateImpl
{
public:
	explicit TVoxelFutureValueState(const FName DebugName)
		: FVoxelFutureValueStateImpl(FVoxelPinType::Make<T>(), DebugName)
	{
	}

	void SetValue(const TSharedRef<const T>& NewValue)
	{
		FVoxelFutureValueStateImpl::SetValue(FVoxelRuntimePinValue::Make(NewValue));
	}
};

FORCEINLINE FVoxelFutureValueStateImpl& FVoxelFutureValueState::GetStateImpl()
{
	checkVoxelSlow(bHasComplexState);
	return static_cast<FVoxelFutureValueStateImpl&>(*this);
}