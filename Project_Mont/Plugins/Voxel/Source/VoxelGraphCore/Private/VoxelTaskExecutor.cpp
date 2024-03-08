// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTaskExecutor.h"
#include "VoxelThreadPool.h"
#include "VoxelMemoryScope.h"
#include "VoxelTaskGroupScope.h"
#include "Engine/Engine.h"
#include "HAL/RunnableThread.h"
#if WITH_EDITOR
#include "LevelEditorViewport.h"
#endif

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelBenchmark, false,
	"voxel.Benchmark",
	"If true, will continuously refresh the world as soon as it's done processing");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelLogTaskTimes, false,
	"voxel.LogTaskTimes",
	"If true, will log how long it's been since the task queue was empty");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, float, GVoxelThreadingPriorityDuration, 0.5f,
	"voxel.threading.PriorityDuration",
	"Task priorities will be recomputed with the new camera position every PriorityDuration seconds");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, int32, GVoxelThreadingMaxSortedTasks, 4096,
	"voxel.threading.MaxSortedTasks",
	"");

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelLogTasks, false,
	"voxel.LogTasks",
	"Will log whenever a new task is queued");

VOXEL_CONSOLE_COMMAND(
	LogAllTasks,
	"voxel.LogAllTasks",
	"")
{
	GVoxelTaskExecutor->LogAllTasks();
}

FVoxelTaskExecutor* GVoxelTaskExecutor = new FVoxelTaskExecutor();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTaskExecutor::LogAllTasks()
{
	Groups.ForeachGroup([&](const FVoxelTaskGroup& Group)
	{
		Group.LogTasks();
		return true;
	});
}

void FVoxelTaskExecutor::AddGroup(const TSharedRef<FVoxelTaskGroup>& Group)
{
	if (IsExiting())
	{
		return;
	}

	if (Groups.Num() == 0)
	{
		RunOnGameThread([this]
		{
			OnBeginProcessing.Broadcast();
		});
	}

	if (GVoxelLogTasks)
	{
		LOG_VOXEL(Log, "New task queued: %s", *Group->Name.ToString());
	}

	Groups.Add(Group);
	Event.Trigger();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTaskExecutor::Initialize()
{
	TFunction<void()> Callback = [this]
	{
		bIsExiting.Set(true);
		Groups.Reset();

		VOXEL_SCOPE_LOCK(ThreadsCriticalSection);
		Threads.Reset();
	};

	FCoreDelegates::OnPreExit.AddLambda(Callback);
	FCoreDelegates::OnExit.AddLambda(Callback);
	GOnVoxelModuleUnloaded_DoCleanup.AddLambda(Callback);
}

void FVoxelTaskExecutor::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	if (IsExiting())
	{
		return;
	}

	const int32 CurrentNumTasks = NumTasks();

	if (CurrentNumTasks > 0)
	{
		AsyncBackgroundTask([this]
		{
			VOXEL_SCOPE_COUNTER("Update NumTasks");

			Groups.ForeachGroup([&](FVoxelTaskGroup& Group)
			{
				return true;
			});
		});
	}

	if (!GVoxelHideTaskCount &&
		CurrentNumTasks > 0)
	{
		const FString Message = FString::Printf(TEXT("%d voxel tasks left using %d threads"), CurrentNumTasks, GVoxelNumThreads);
		GEngine->AddOnScreenDebugMessage(uint64(0x557D0C945D26), FApp::GetDeltaTime() * 1.5f, FColor::White, Message);

#if WITH_EDITOR
		extern UNREALED_API FLevelEditorViewportClient* GCurrentLevelEditingViewportClient;
		if (GCurrentLevelEditingViewportClient)
		{
			GCurrentLevelEditingViewportClient->SetShowStats(true);
		}
#endif
	}

	if (GVoxelLogTaskTimes &&
		bWasProcessingTaskLastFrame &&
		CurrentNumTasks == 0)
	{
		LOG_VOXEL(Log, "Tasks took %.3fs", FPlatformTime::Seconds() - LastNoTasksTime);
	}

	if (GVoxelBenchmark &&
		bWasProcessingTaskLastFrame &&
		CurrentNumTasks == 0)
	{
		const double Time = FPlatformTime::Seconds() - LastNoTasksTime;
		LOG_VOXEL(Log, "Tasks took %.3fs", Time);
		GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, FString::Printf(TEXT("Tasks took %.3fs"), Time));

		GEngine->Exec(nullptr, TEXT("voxel.RefreshAll"));
	}

	if (CurrentNumTasks == 0)
	{
		LastNoTasksTime = FPlatformTime::Seconds();

		if (bWasProcessingTaskLastFrame)
		{
			OnEndProcessing.Broadcast();
		}
	}
	bWasProcessingTaskLastFrame = CurrentNumTasks > 0;

	GVoxelNumThreads = FMath::Clamp(GVoxelNumThreads, 1, 128);

	if (Threads.Num() != GVoxelNumThreads)
	{
		AsyncBackgroundTask([this]
		{
			VOXEL_SCOPE_LOCK(ThreadsCriticalSection);

			while (Threads.Num() < GVoxelNumThreads)
			{
				Threads.Add(MakeUnique<FThread>());
				Event.Trigger();
			}

			while (Threads.Num() > GVoxelNumThreads)
			{
				Threads.Pop();
			}
		});
	}

	VOXEL_SCOPE_COUNTER("Processing GameThread tasks");

	const double StartTime = FPlatformTime::Seconds();

	TVoxelUniqueFunction<void()> Task;
	while (GameThreadTaskQueue.Dequeue(Task))
	{
		Task();

		if (FPlatformTime::Seconds() - StartTime > 0.1)
		{
			LOG_VOXEL(Warning, "More then 0.1s spent processing game tasks - trottling");
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTaskExecutor::FTaskGroupArray::FTaskGroupArray()
{
	Groups.Reserve(2048);
}

void FVoxelTaskExecutor::FTaskGroupArray::Add(const TSharedRef<FVoxelTaskGroup>& NewGroup)
{
	VOXEL_FUNCTION_COUNTER();
	FVoxelScopeLock_Write Lock(CriticalSection);

	if (GVoxelTaskExecutor->IsExiting())
	{
		return;
	}

#if VOXEL_DEBUG && 0
	for (const FTaskGroup& Group : Groups)
	{
		ensure(Group.Data->WeakGroup != NewGroup);
	}
#endif

	FTaskGroup TaskGroup(NewGroup);

	if (Groups.Num() > GVoxelThreadingMaxSortedTasks)
	{
		bSorted = false;
	}

	if (bSorted)
	{
		const int32 InsertLocation = VOXEL_INLINE_COUNTER("LowerBound", Algo::LowerBound(Groups, TaskGroup, TLess<>()));

		VOXEL_SCOPE_COUNTER("Insert");
		Groups.Insert(MoveTemp(TaskGroup), InsertLocation);
		//checkVoxelSlow(Algo::IsSorted(Groups));
	}
	else
	{
		Groups.Add(MoveTemp(TaskGroup));
	}

	ValidGroups.Reset();
	ValidGroups.SetNum(Groups.Num(), true);

	NumGroups.Set(Groups.Num());
}

void FVoxelTaskExecutor::FTaskGroupArray::Reset()
{
	FVoxelScopeLock_Write Lock(CriticalSection);
	bSorted = true;
	LastPriorityComputeTime = 0;
	Groups.Reset();
	ValidGroups.Reset();
}

void FVoxelTaskExecutor::FTaskGroupArray::UpdatePriorities()
{
	VOXEL_FUNCTION_COUNTER();
	FVoxelScopeLock_Write Lock(CriticalSection);

	LastPriorityComputeTime = FPlatformTime::Seconds();

	Groups.RemoveAllSwap([](const FTaskGroup& TaskGroup)
	{
		return !TaskGroup.Data->WeakGroup.IsValid();
	}, false);

	ValidGroups.Reset();
	ValidGroups.SetNum(Groups.Num(), true);

	NumGroups.Set(Groups.Num());

	if (Groups.Num() > GVoxelThreadingMaxSortedTasks)
	{
		bSorted = false;
		return;
	}

	bSorted = true;

	ParallelFor(Groups, [&](FTaskGroup& TaskGroup)
	{
		TaskGroup.PriorityValue = TaskGroup.Data->Priority.GetPriority();
	});

	VOXEL_SCOPE_COUNTER("Sort");
	Groups.Sort();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTaskExecutor::FThread::FThread()
{
	UE::Trace::ThreadGroupBegin(TEXT("VoxelThreadPool"));

	static int32 ThreadIndex = 0;
	const FString Name = FString::Printf(TEXT("Voxel Thread %d"), ThreadIndex++);

	Thread = FRunnableThread::Create(
		this,
		*Name,
		1024 * 1024 * (GVoxelBypassTaskQueue ? 128 : 1),
		EThreadPriority(FMath::Clamp(GVoxelThreadPriority, 0, 6)),
		FPlatformAffinity::GetPoolThreadMask());

	UE::Trace::ThreadGroupEnd();
}

FVoxelTaskExecutor::FThread::~FThread()
{
	VOXEL_FUNCTION_COUNTER();

	// Tell the thread it needs to die
	bTimeToDie.Set(true);
	// Trigger the thread so that it will come out of the wait state if
	// it isn't actively doing work
	GVoxelTaskExecutor->Event.Trigger();
	// Kill (but wait for thread to finish)
	Thread->Kill(true);
	// Delete
	delete Thread;
}

uint32 FVoxelTaskExecutor::FThread::Run()
{
	VOXEL_LLM_SCOPE();

	const TUniquePtr<FVoxelMemoryScope> MemoryScope = MakeUnique<FVoxelMemoryScope>();

Wait:
	if (bTimeToDie.Get())
	{
		return 0;
	}

	if (!VOXEL_ALLOW_MALLOC_INLINE(GVoxelTaskExecutor->Event.Wait(10)))
	{
		MemoryScope->Clear();
		goto Wait;
	}

GetNextTask:
	if (bTimeToDie.Get())
	{
		return 0;
	}

	TSharedPtr<FVoxelTaskGroup> TmpGroup = GVoxelTaskExecutor->GetGroupToProcess(this);
	if (!TmpGroup)
	{
		goto Wait;
	}

	FVoxelTaskGroupScope Scope;
	if (!Scope.Initialize(*TmpGroup))
	{
		// Exiting
		goto Wait;
	}

	// Reset to only have one valid group ref for ShouldExit
	TmpGroup.Reset();

	checkVoxelSlow(Scope.GetGroup().ActiveThread.Get() == this);

	Scope.GetGroup().ProcessTasks();

	const void* Old = Scope.GetGroup().ActiveThread.Exchange(nullptr);
	checkVoxelSlow(Old == this);

	goto GetNextTask;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelTaskGroup> FVoxelTaskExecutor::GetGroupToProcess(const FThread* Thread)
{
	VOXEL_FUNCTION_COUNTER();

	TSharedPtr<FVoxelTaskGroup> GroupToProcess;
	Groups.ForeachGroup([&](FVoxelTaskGroup& Group)
	{
		if (!Group.HasTasks())
		{
			return true;
		}

		const void* Expected = nullptr;
		if (!Group.ActiveThread.CompareExchangeStrong(Expected, Thread))
		{
			return true;
		}
		checkVoxelSlow(Expected == nullptr);

		ensure(!GroupToProcess);
		GroupToProcess = Group.AsShared();
		return false;
	});

	return GroupToProcess;
}