// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTaskGroup.h"
#include "VoxelTaskExecutor.h"
#include "VoxelTaskGroupScope.h"
#include "VoxelTerminalGraphInstance.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelTaskGroup);

const uint32 GVoxelTaskGroupTLS = FPlatformTLS::AllocTlsSlot();

TSharedRef<FVoxelTaskGroup> FVoxelTaskGroup::Create(
	const FName Name,
	const FVoxelTaskPriority& Priority,
	const TSharedRef<FVoxelTaskReferencer>& Referencer,
	const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance)
{
	const TSharedRef<FVoxelTaskGroup> Group = MakeVoxelShareable(new (GVoxelMemory) FVoxelTaskGroup(
		Name,
		false,
		Priority,
		Referencer,
		TerminalGraphInstance));
	GVoxelTaskExecutor->AddGroup(Group);
	return Group;
}

TSharedRef<FVoxelTaskGroup> FVoxelTaskGroup::CreateSynchronous(
	const FName Name,
	const TSharedRef<FVoxelTaskReferencer>& Referencer,
	const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance)
{
	return MakeVoxelShareable(new (GVoxelMemory) FVoxelTaskGroup(
		Name,
		true,
		{},
		Referencer,
		TerminalGraphInstance));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTaskGroup::LogTasks() const
{
	VOXEL_FUNCTION_COUNTER();

	LOG_VOXEL(Log, "%s", *Name.ToString());

	if (HasTasks())
	{
		LOG_VOXEL(Log, "\tHas queued tasks");
	}

	VOXEL_SCOPE_LOCK(PendingTasksCriticalSection);

	PendingTasks_RequiresLock.Foreach([&](const TVoxelUniquePtr<FVoxelTask>& PendingTask)
	{
		LOG_VOXEL(Log, "\tPending task %s: %d dependencies", *PendingTask->Name.ToString(), PendingTask->NumDependencies.Get());
	});
}

void FVoxelTaskGroup::ProcessTasks()
{
	VOXEL_SCOPE_COUNTER_FNAME(Name);
	VOXEL_SCOPE_COUNTER_FNAME_COND(!InstanceStatName.IsNone(), InstanceStatName);
	VOXEL_SCOPE_COUNTER_FNAME_COND(!GraphStatName.IsNone(), GraphStatName);
	VOXEL_SCOPE_COUNTER_FNAME_COND(!CallstackStatName.IsNone(), CallstackStatName);
	checkVoxelSlow(ActiveThread.Get());
	checkVoxelSlow(&Get() == this);

	checkVoxelSlow(ActiveThreadId.Get() == -1);
	ActiveThreadId.Set(FPlatformTLS::GetCurrentThreadId());
	ON_SCOPE_EXIT
	{
		ActiveThreadId.Set(-1);
	};

	ensure(!RootTag);
	ON_SCOPE_EXIT
	{
		RootTag.Reset();
	};

	const FVoxelQueryScope Scope(nullptr, &TerminalGraphInstance.Get());

	while (HasTasks())
	{
		while (Tasks_Array.Num() > 0)
		{
			if (ShouldExit())
			{
				return;
			}

			const TVoxelUniquePtr<FVoxelTask> Task = Tasks_Array.Pop();
			Task->Execute();
		}

		TVoxelUniquePtr<FVoxelTask> Task;
		while (Tasks_Queue.Dequeue(Task))
		{
			Tasks_Array.Add(MoveTemp(Task));
		}
	}
}

bool FVoxelTaskGroup::TryRunSynchronously(FString* OutError)
{
	VOXEL_FUNCTION_COUNTER();
	check(bIsSynchronous);

	checkVoxelSlow(!ActiveThread.Get());
	ActiveThread.Set(reinterpret_cast<const void*>(-1));
	{
		ProcessTasks();
	}
	ActiveThread.Set(nullptr);

	if (ShouldExit())
	{
		if (OutError)
		{
			*OutError = "Exit requested";
		}
		return false;
	}

	VOXEL_SCOPE_LOCK(PendingTasksCriticalSection);

	if (PendingTasks_RequiresLock.Num() == 0)
	{
		return true;
	}

	if (OutError)
	{
		TVoxelArray<FString> PendingTaskNames;
		PendingTasks_RequiresLock.Foreach([&](const TVoxelUniquePtr<FVoxelTask>& PendingTask)
		{
			PendingTaskNames.Add(PendingTask->Name.ToString());
		});
		*OutError = "Failed to process tasks synchronously. Tasks left: " + FString::Join(PendingTaskNames, TEXT(","));
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTaskGroup::QueueTask_Async(TVoxelUniquePtr<FVoxelTask> TaskPtr)
{
	Tasks_Queue.Enqueue(MoveTemp(TaskPtr));

	if (ActiveThread.Get(std::memory_order_relaxed))
	{
		return;
	}

	VOXEL_SCOPE_COUNTER("Trigger");
	GVoxelTaskExecutor->Event.Trigger();
}

void FVoxelTaskGroup::QueueTask_BypassTaskQueue(TVoxelUniquePtr<FVoxelTask> TaskPtr)
{
	FVoxelTaskGroupScope Scope;
	if (!Scope.Initialize(*this))
	{
		// Exiting
		return;
	}

	TaskPtr->Execute();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTaskGroup::AddPendingTask(TVoxelUniquePtr<FVoxelTask> TaskPtr)
{
	VOXEL_SCOPE_LOCK(PendingTasksCriticalSection);

	FVoxelTask& Task = *TaskPtr;
	checkVoxelSlow(Task.NumDependencies.Get() > 0);

	checkVoxelSlow(Task.PendingTaskIndex == -1);
	Task.PendingTaskIndex = PendingTasks_RequiresLock.Add(MoveTemp(TaskPtr));
}

TVoxelUniquePtr<FVoxelTask> FVoxelTaskGroup::RemovePendingTask(FVoxelTask& Task)
{
	VOXEL_SCOPE_LOCK(PendingTasksCriticalSection);
	checkVoxelSlow(Task.NumDependencies.Get() == 0);
	checkVoxelSlow(PendingTasks_RequiresLock[Task.PendingTaskIndex].Get() == &Task);

	const int32 Index = Task.PendingTaskIndex;
	Task.PendingTaskIndex = -1;

	TVoxelUniquePtr<FVoxelTask> TaskPtr = MoveTemp(PendingTasks_RequiresLock[Index]);
	PendingTasks_RequiresLock.RemoveAt(Index);
	return TaskPtr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTaskGroup::SetTagNames(const TConstVoxelArrayView<FName> TagNames)
{
	checkVoxelSlow(ActiveThread.Get());
	ensureVoxelSlow(AreVoxelStatsEnabled());

	if (GVoxelBypassTaskQueue)
	{
		return;
	}

	TSharedPtr<FTag>* TagPtr = &RootTag;
	int32 Index = 0;
	while (true)
	{
		if (!TagNames.IsValidIndex(Index))
		{
			TagPtr->Reset();
			break;
		}

		// TagNames are reversed
		const FName TagName = TagNames.Last(Index);

		if (!TagPtr->IsValid() ||
			TagPtr->Get()->Name != TagName)
		{
			TagPtr->Reset();
			*TagPtr = MakeVoxelShared<FTag>(TagName);
		}

		TagPtr = &TagPtr->Get()->ChildTag;
		Index++;
	}
}

FVoxelTaskGroup::FTag::FTag(const FName Name)
	: Name(Name)
{
	ensure(!Name.IsNone());

#if CPUPROFILERTRACE_ENABLED
	FCpuProfilerTrace::OutputBeginDynamicEvent(Name, __FILE__, __LINE__);
#endif
}

FVoxelTaskGroup::FTag::~FTag()
{
	// Destroy child first so it emits its OutputEndEvent
	ChildTag.Reset();

#if CPUPROFILERTRACE_ENABLED
	FCpuProfilerTrace::OutputEndEvent();
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTaskGroup::FVoxelTaskGroup(
	const FName Name,
	const bool bIsSynchronous,
	const FVoxelTaskPriority& Priority,
	const TSharedRef<FVoxelTaskReferencer>& Referencer,
	const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance)
	: Name(Name)
	, InstanceStatName(TerminalGraphInstance->RuntimeInfo->GetInstanceName())
	, GraphStatName(TerminalGraphInstance->RuntimeInfo->GetGraphName())
	, CallstackStatName(TerminalGraphInstance->Callstack->ToDebugString())
	, bIsSynchronous(bIsSynchronous)
	, Priority(Priority)
	, Referencer(Referencer)
	, RuntimeInfo(TerminalGraphInstance->RuntimeInfo)
	, TerminalGraphInstance(TerminalGraphInstance)
{
	VOXEL_FUNCTION_COUNTER();

	Tasks_Array.Reserve(512);
	PendingTasks_RequiresLock.Reserve(512);
}