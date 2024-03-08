// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelDynamicValue.h"
#include "VoxelTaskGroup.h"
#include "VoxelTaskGroupScope.h"
#include "VoxelDependencyTracker.h"

class VOXELGRAPHCORE_API FVoxelDynamicValueState final
	: public FVoxelDynamicValueStateBase
	, public TSharedFromThis<FVoxelDynamicValueState>
{
public:
	const TSharedRef<const FVoxelComputeValue> ComputeLambda;
	const FName Name;
	const FVoxelPinType Type;
	const bool bRunSynchronously;
	const FVoxelTaskPriority Priority;
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance;
	const TSharedRef<const FVoxelQueryParameters> Parameters;
	const TSharedRef<FVoxelTaskReferencer> Referencer;

	VOXEL_COUNT_INSTANCES();
	UE_NONCOPYABLE(FVoxelDynamicValueState);

	virtual FVoxelPinType GetType() const override
	{
		return Type;
	}
	virtual void SetOnChanged(FOnChanged&& NewOnChanged) const override
	{
		OnChangedImpl->SetOnChanged(MoveTemp(NewOnChanged));
	}

private:
	FVoxelDynamicValueState(
		const TSharedRef<const FVoxelComputeValue>& ComputeLambda,
		const FName Name,
		const FVoxelPinType& Type,
		const bool bRunSynchronously,
		const FVoxelTaskPriority& Priority,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		const TSharedRef<const FVoxelQueryParameters>& Parameters,
		const TSharedRef<FVoxelTaskReferencer>& Referencer)
		: ComputeLambda(ComputeLambda)
		, Name(Name)
		, Type(Type)
		, bRunSynchronously(bRunSynchronously)
		, Priority(Priority)
		, TerminalGraphInstance(TerminalGraphInstance)
		, Parameters(Parameters)
		, Referencer(Referencer)
		, OnChangedImpl(MakeVoxelShared<FOnChangedImpl>(Name))
	{
	}

	FVoxelCriticalSection CriticalSection;
	TSharedPtr<FVoxelTaskGroup> Group;
	TSharedPtr<FVoxelDependencyTracker> DependencyTracker;
	int32 ValueCounter = 0;

	class FOnChangedImpl : public TSharedFromThis<FOnChangedImpl>
	{
	public:
		const FName Name;

		explicit FOnChangedImpl(const FName Name)
			: Name(Name)
		{
		}

		void SetOnChanged(FOnChanged&& NewOnChanged);
		void Execute(const FVoxelRuntimePinValue& NewValue, int32 NewValueCounter);

	private:
		FVoxelCriticalSection CriticalSection;
		FOnChanged OnChanged;
		int32 LastValueCounter = -1;
		FVoxelRuntimePinValue PendingValue;
	};
	const TSharedRef<FOnChangedImpl> OnChangedImpl;

	void Compute_RequiresLock_WillUnlock();
	void OnComputed(
		FVoxelRuntimePinValue NewValue,
		const TSharedRef<FVoxelDependencyTracker>& NewDependencyTracker);

	friend class FVoxelDynamicValueFactoryBase;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelDynamicValueState);

void FVoxelDynamicValueState::FOnChangedImpl::SetOnChanged(FOnChanged&& NewOnChanged)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);
	ensureMsgf(!OnChanged, TEXT("Previous OnChanged will be cleared! %s was likely copied by mistake"), *Name.ToString());

	OnChanged = MoveTemp(NewOnChanged);

	if (!PendingValue.IsValid())
	{
		return;
	}

	const FVoxelRuntimePinValue PendingValueCopy = PendingValue;
	PendingValue = {};
	OnChanged(PendingValueCopy);
}

void FVoxelDynamicValueState::FOnChangedImpl::Execute(const FVoxelRuntimePinValue& NewValue, const int32 NewValueCounter)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	if (LastValueCounter > NewValueCounter)
	{
		// We already have a more recent value
		return;
	}
	LastValueCounter = NewValueCounter;

	if (!OnChanged)
	{
		PendingValue = NewValue;
		return;
	}

	OnChanged(NewValue);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelDynamicValueState::Compute_RequiresLock_WillUnlock()
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(CriticalSection.IsLocked());

	if (State == EState::UpToDate)
	{
		State = EState::Outdated;
	}

	ensure(!Group);
	if (bRunSynchronously)
	{
		Group = FVoxelTaskGroup::CreateSynchronous(Name, Referencer, TerminalGraphInstance);
	}
	else
	{
		Group = FVoxelTaskGroup::Create(Name, Priority, Referencer, TerminalGraphInstance);
	}

	DependencyTracker.Reset();

	// In case the voxel task is ran immediately if Thread is GameThread
	CriticalSection.Unlock();

	FVoxelTaskGroupScope Scope;
	if (!Scope.Initialize(*Group))
	{
		// Exiting
		return;
	}

	MakeVoxelTask("Setup")
	.Execute(MakeWeakPtrLambda(this, [this]
	{
		const TSharedRef<FVoxelDependencyTracker> NewDependencyTracker = FVoxelDependencyTracker::Create(Name);
		const FVoxelQuery Query = FVoxelQuery::Make(
			TerminalGraphInstance,
			Parameters,
			NewDependencyTracker);

		FVoxelFutureValue FutureValue = (*ComputeLambda)(Query);
		if (!FutureValue.IsValid())
		{
			FutureValue = FVoxelRuntimePinValue(Type);
		}

		MakeVoxelTask()
		.Dependency(FutureValue)
		.Execute(MakeWeakPtrLambda(this, [this, NewDependencyTracker, FutureValue]
		{
			OnComputed(FutureValue.GetValue_CheckCompleted(), NewDependencyTracker);
		}));
	}));

	if (!bRunSynchronously)
	{
		return;
	}

	FString Error;
	if (!Group->TryRunSynchronously(&Error))
	{
		ensureMsgf(Group->ShouldExit(), TEXT("Failed to run %s synchronously: %s"), *Name.ToString(), *Error);
	}
}

void FVoxelDynamicValueState::OnComputed(
	FVoxelRuntimePinValue NewValue,
	const TSharedRef<FVoxelDependencyTracker>& NewDependencyTracker)
{
	VOXEL_FUNCTION_COUNTER();
	CriticalSection.Lock();

	if (!ensure(NewValue.GetType().CanBeCastedTo(Type)))
	{
		NewValue = FVoxelRuntimePinValue(Type);
	}

	ensure(State != EState::UpToDate);
	State = EState::UpToDate;

	ensure(Group);
	Group = {};

	ensure(!DependencyTracker);
	DependencyTracker = NewDependencyTracker;

	OnChangedImpl->Execute(NewValue, ValueCounter++);

	if (!DependencyTracker->TrySetOnInvalidated(MakeWeakPtrLambda(this,
		[this]
		{
			CriticalSection.Lock();

			if (State != EState::UpToDate ||
				!ensure(DependencyTracker) ||
				!ensure(DependencyTracker->IsInvalidated()))
			{
				CriticalSection.Unlock();
				return;
			}

			Compute_RequiresLock_WillUnlock();
		})))
	{
		// Already invalidated, recompute
		ensure(DependencyTracker->IsInvalidated());
		Compute_RequiresLock_WillUnlock();
	}
	else
	{
		CriticalSection.Unlock();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelDynamicValueStateBase> FVoxelDynamicValueFactoryBase::ComputeState(
	const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
	const TSharedRef<const FVoxelQueryParameters>& Parameters) const
{
	ensure(Compute);
	ensure(!Name.IsNone());
	ensure(Type.IsValid());
	ensure(Referencer);

	const TSharedRef<FVoxelDynamicValueState> State = MakeVoxelShareable(new (GVoxelMemory) FVoxelDynamicValueState(
		Compute.ToSharedRef(),
		Name,
		Type,
		bPrivateRunSynchronously,
		PrivatePriority,
		TerminalGraphInstance,
		Parameters,
		Referencer.ToSharedRef()));

	State->CriticalSection.Lock();
	State->Compute_RequiresLock_WillUnlock();
	return State;
}