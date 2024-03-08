// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTask.h"
#include "VoxelNode.h"
#include "VoxelTaskGroup.h"
#include "VoxelTaskExecutor.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelFutureValueState.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelBypassTaskQueue, false,
	"voxel.BypassTaskQueue",
	"If true, will bypass task queue whenever possible, making debugging easier");

VOXEL_RUN_ON_STARTUP_GAME(InitializeVoxelBypassTaskQueue)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("VoxelBypassTaskQueue")))
	{
		GVoxelBypassTaskQueue = true;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool ShouldExitVoxelTask()
{
	return FVoxelTaskGroup::Get().ShouldExit();
}

bool ShouldRunVoxelTaskInParallel()
{
	if (IsInGameThreadFast())
	{
		return true;
	}

	const FVoxelTaskGroup* Group = FVoxelTaskGroup::TryGet();
	if (!ensureVoxelSlow(Group))
	{
		return false;
	}
	return Group->RuntimeInfo->ShouldRunTasksInParallel();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelTaskPriorityTicker : public FVoxelSingleton
{
public:
	FVoxelCriticalSection CriticalSection;
	TVoxelMap<FObjectKey, TVoxelMap<FVoxelTransformRef, TWeakPtr<FVector>>> WorldToLocalToWorldToPosition;

	//~ Begin FVoxelSingleton Interface
	virtual void Tick() override
	{
		VOXEL_FUNCTION_COUNTER();
		VOXEL_SCOPE_LOCK(CriticalSection);

		for (auto WorldIt = WorldToLocalToWorldToPosition.CreateIterator(); WorldIt; ++WorldIt)
		{
			const UWorld* World = CastChecked<UWorld>(WorldIt.Key().ResolveObjectPtr(), ECastCheckedType::NullAllowed);
			if (!World)
			{
				WorldIt.RemoveCurrent();
				continue;
			}

			FVector CameraPosition = FVector::ZeroVector;
			FVoxelUtilities::GetCameraView(World, CameraPosition);

			for (auto It = WorldIt.Value().CreateIterator(); It; ++It)
			{
				const TSharedPtr<FVector> PositionRef = It.Value().Pin();
				if (!PositionRef)
				{
					It.RemoveCurrent();
					continue;
				}

				const FMatrix LocalToWorld = It.Key().Get_NoDependency();
				*PositionRef = LocalToWorld.InverseTransformPosition(CameraPosition);
			}
		}
	}
	//~ End FVoxelSingleton Interface
};
FVoxelTaskPriorityTicker* GVoxelTaskPriorityTicker = new FVoxelTaskPriorityTicker();

TSharedRef<const FVector> FVoxelTaskPriority::GetPosition(
	const FObjectKey World,
	const FVoxelTransformRef& LocalToWorld)
{
	VOXEL_SCOPE_LOCK(GVoxelTaskPriorityTicker->CriticalSection);

	TVoxelMap<FVoxelTransformRef, TWeakPtr<FVector>>& LocalToWorldToPosition = GVoxelTaskPriorityTicker->WorldToLocalToWorldToPosition.FindOrAdd(World);
	TWeakPtr<FVector>& WeakPosition = LocalToWorldToPosition.FindOrAdd(LocalToWorld);

	TSharedPtr<FVector> Position = WeakPosition.Pin();
	if (!Position)
	{
		Position = MakeVoxelShared<FVector>(FVector::ZAxisVector);
		WeakPosition = Position;
	}
	return Position.ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelTaskReferencer);

FVoxelTaskReferencer::FVoxelTaskReferencer(const FName Name)
	: Name(Name)
{
	VOXEL_FUNCTION_COUNTER();
	Objects_RequiresLock.Reserve(16);
	GraphEvaluators_RequiresLock.Reserve(16);
}

void FVoxelTaskReferencer::AddRef(const TSharedRef<const FVoxelVirtualStruct>& Object)
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	Objects_RequiresLock.Add(Object);
}

void FVoxelTaskReferencer::AddEvaluator(const TSharedRef<const FVoxelGraphEvaluator>& GraphEvaluator)
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	GraphEvaluators_RequiresLock.Add(GraphEvaluator);
}

bool FVoxelTaskReferencer::IsReferenced(const FVoxelVirtualStruct* Object) const
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	if (Objects_RequiresLock.Contains(GetTypeHash(Object), [&](const TSharedPtr<const FVoxelVirtualStruct>& SharedObject)
		{
			return SharedObject.Get() == Object;
		}))
	{
		return true;
	}

	if (const FVoxelNode* Node = Cast<FVoxelNode>(Object))
	{
		if (Node->GetNodeRuntime().IsCallNode())
		{
			return true;
		}

		for (const TSharedPtr<const FVoxelGraphEvaluator>& GraphEvaluator : GraphEvaluators_RequiresLock)
		{
			if (GraphEvaluator->HasNode(*Node))
			{
				return true;
			}
		}
	}

	return false;
}

FVoxelTaskReferencer& FVoxelTaskReferencer::Get()
{
	return FVoxelTaskGroup::Get().GetReferencer();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelTask);

#if VOXEL_DEBUG
void FVoxelTask::CheckIsSafeToExecute() const
{
	check(NumDependencies.Get() == 0);
	check(Group.RuntimeInfo->NumActiveTasks.Get() > 0);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTaskFactory::Execute(TVoxelUniqueFunction<void()> Lambda)
{
	ensureVoxelSlow(!PrivateName.IsNone());

	if (bPrivateRunOnGameThread)
	{
		Lambda = MakeGameThreadLambda(MoveTemp(Lambda));
	}
	if (AreVoxelStatsEnabled())
	{
		Lambda = MakeStatsLambda(MoveTemp(Lambda));
	}

	FVoxelTaskGroup& Group = FVoxelTaskGroup::Get();

	TVoxelUniquePtr<FVoxelTask> TaskPtr = MakeVoxelUnique<FVoxelTask>(
		Group,
		PrivateName,
		MoveTemp(Lambda));

	if (DependenciesView.Num() == 0)
	{
		Group.QueueTask(MoveTemp(TaskPtr));
		return;
	}

	FVoxelTask& Task = *TaskPtr;

	// Initialize to 1000 to avoid having a parallel task setting NumDependencies to 0 & queuing the task
	Task.NumDependencies.Set(1000);

	for (const FVoxelFutureValue& Dependency : DependenciesView)
	{
		if (!ensureMsgfVoxelSlow(Dependency.IsValid(), TEXT("Invalid dependency passed to %s"), *PrivateName.ToString()) ||
			Dependency.State->IsComplete(std::memory_order_relaxed))
		{
			continue;
		}

		if (!Dependency.State->GetStateImpl().TryAddDependentTask(Task))
		{
			// Completed
			continue;
		}

		Task.NumDependencies.Increment();
	}

	if (Task.NumDependencies.Get() == 1000)
	{
		// No dependencies
		Task.NumDependencies.Set(0);
		Group.QueueTask(MoveTemp(TaskPtr));
		return;
	}

	Group.AddPendingTask(MoveTemp(TaskPtr));

	const int32 NumDependenciesLeft = Task.NumDependencies.Subtract_ReturnNew(1000);
	checkVoxelSlow(NumDependenciesLeft >= 0);

	if (NumDependenciesLeft > 0)
	{
		// Queued, can't execute
		return;
	}

	// Dependencies are already completed, execute
	TaskPtr = Group.RemovePendingTask(Task);
	Group.QueueTask(MoveTemp(TaskPtr));
}

FVoxelFutureValue FVoxelTaskFactory::Execute(const FVoxelPinType& Type, TVoxelUniqueFunction<FVoxelFutureValue()>&& Lambda)
{
	checkVoxelSlow(Type.IsValid());
	checkVoxelSlow(!Type.IsWildcard());

	const TSharedRef<FVoxelFutureValueStateImpl> State = MakeVoxelShared<FVoxelFutureValueStateImpl>(Type, PrivateName);

	Execute([State, Name = PrivateName, Lambda = MoveTemp(Lambda)]
	{
		const FVoxelFutureValue Value = Lambda();

		if (!Value.IsValid())
		{
			State->SetValue(FVoxelRuntimePinValue(State->Type));
			return;
		}

#if WITH_EDITOR
		if (!ensure(Value.GetParentType().CanBeCastedTo(State->Type)))
		{
			State->SetValue(FVoxelRuntimePinValue(State->Type));
			return;
		}
#endif

		if (!Value.IsComplete())
		{
			if (Value.State->GetStateImpl().TryAddLinkedState(State))
			{
				return;
			}
		}
		checkVoxelSlow(Value.IsComplete());

		const FVoxelRuntimePinValue& FinalValue = Value.GetValue_CheckCompleted();

#if WITH_EDITOR
		if (!ensureMsgf(FinalValue.IsValidValue_Slow(), TEXT("Invalid value produced by %s"), *Name.ToString()))
		{
			State->SetValue(FVoxelRuntimePinValue(State->Type));
			return;
		}
#endif

		State->SetValue(FinalValue);
	});

	return FVoxelFutureValue(State);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelUniqueFunction<void()> FVoxelTaskFactory::MakeStatsLambda(TVoxelUniqueFunction<void()> Lambda) const
{
	if (GVoxelBypassTaskQueue)
	{
		return [
			Name = PrivateName,
			Lambda = MoveTemp(Lambda)]
		{
			VOXEL_SCOPE_COUNTER_FNAME(Name);
			Lambda();
		};
	}

	TVoxelArray<FName> TagNames;
	if (const FVoxelTaskTagScope* Scope = FVoxelTaskTagScope::Get())
	{
		TagNames = Scope->GetTagNames();
	}

	return [
		Name = PrivateName,
		Lambda = MoveTemp(Lambda),
		TagNames = MoveTemp(TagNames)]
	{
		ensure(!FVoxelTaskTagScope::Get());

		FVoxelTaskTagScope Scope;
		Scope.TagNameOverrides = TagNames;
		FVoxelTaskGroup::Get().SetTagNames(TagNames);

		VOXEL_SCOPE_COUNTER_FNAME(Name);
		Lambda();
	};
}

TVoxelUniqueFunction<void()> FVoxelTaskFactory::MakeGameThreadLambda(TVoxelUniqueFunction<void()> Lambda)
{
	return [Lambda = MoveTemp(Lambda)]
	{
		if (IsInGameThreadFast())
		{
			Lambda();
			return;
		}

		GVoxelTaskExecutor->AddGameThreadTask(MoveTemp(ConstCast(Lambda)));
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const uint32 GVoxelTaskTagTLS = FPlatformTLS::AllocTlsSlot();

TVoxelUniquePtr<FVoxelTaskTagScope> FVoxelTaskTagScope::CreateImpl(const FName TagName)
{
	FVoxelTaskTagScope* Scope = new (GVoxelMemory) FVoxelTaskTagScope();
	Scope->TagName = TagName;
	return TVoxelUniquePtr<FVoxelTaskTagScope>(Scope);
}

FVoxelTaskTagScope::FVoxelTaskTagScope()
{
	ensureVoxelSlow(AreVoxelStatsEnabled());

	ParentScope = static_cast<FVoxelTaskTagScope*>(FPlatformTLS::GetTlsValue(GVoxelTaskTagTLS));
	FPlatformTLS::SetTlsValue(GVoxelTaskTagTLS, this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString GetVoxelTaskName(
	const FString& FunctionName,
	const int32 Line,
	const FString& Context)
{
	FString Result = "Task: ";
	if (!Context.IsEmpty())
	{
		Result += Context + " ";
	}
	Result += VoxelStats_CleanupFunctionName(FunctionName);
	Result += ":";
	Result += FString::FromInt(Line);
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTaskTagScope::~FVoxelTaskTagScope()
{
	checkVoxelSlow(FPlatformTLS::GetTlsValue(GVoxelTaskTagTLS) == this);
	FPlatformTLS::SetTlsValue(GVoxelTaskTagTLS, ParentScope);
}

TVoxelArray<FName> FVoxelTaskTagScope::GetTagNames() const
{
	TVoxelArray<FName> TagNames;
	for (const FVoxelTaskTagScope* Scope = this; Scope; Scope = Scope->ParentScope)
	{
		if (Scope->TagNameOverrides.IsSet())
		{
			ensure(!Scope->ParentScope);
			TagNames.Append(Scope->TagNameOverrides.GetValue());
			break;
		}

		TagNames.Add(Scope->TagName);
	}
	return TagNames;
}