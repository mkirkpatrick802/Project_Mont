// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTask.h"
#include "VoxelQuery.h"

extern VOXELGRAPHCORE_API const uint32 GVoxelTaskGroupTLS;

class VOXELGRAPHCORE_API FVoxelTaskGroup : public TSharedFromThis<FVoxelTaskGroup>
{
public:
	const FName Name;
	const FName InstanceStatName;
	const FName GraphStatName;
	const FName CallstackStatName;
	const bool bIsSynchronous;
	const FVoxelTaskPriority Priority;
	const TSharedRef<FVoxelTaskReferencer> Referencer;
	const TSharedRef<const FVoxelRuntimeInfo> RuntimeInfo;
	const TSharedRef<const FVoxelTerminalGraphInstance> TerminalGraphInstance;
	TVoxelAtomic<const void*> ActiveThread;

	VOXEL_COUNT_INSTANCES();

public:
	static TSharedRef<FVoxelTaskGroup> Create(
		FName Name,
		const FVoxelTaskPriority& Priority,
		const TSharedRef<FVoxelTaskReferencer>& Referencer,
		const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance);

	static TSharedRef<FVoxelTaskGroup> CreateSynchronous(
		FName Name,
		const TSharedRef<FVoxelTaskReferencer>& Referencer,
		const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance);

public:
	FORCEINLINE static FVoxelTaskGroup& Get()
	{
		void* TLSValue = FPlatformTLS::GetTlsValue(GVoxelTaskGroupTLS);
		checkVoxelSlow(TLSValue);
		return *static_cast<FVoxelTaskGroup*>(TLSValue);
	}
	FORCEINLINE static FVoxelTaskGroup* TryGet()
	{
		void* TLSValue = FPlatformTLS::GetTlsValue(GVoxelTaskGroupTLS);
		return static_cast<FVoxelTaskGroup*>(TLSValue);
	}

public:
	FORCEINLINE FVoxelTaskReferencer& GetReferencer() const
	{
		return *Referencer;
	}
	FORCEINLINE bool HasTasks() const
	{
		return
			Tasks_Array.Num() > 0 ||
			!Tasks_Queue.IsEmpty();
	}
	FORCEINLINE bool ShouldExit() const
	{
		if (RuntimeInfo->ShouldExitTask())
		{
			return true;
		}
		if (bIsSynchronous)
		{
			return false;
		}
		return IsSharedFromThisUnique(*this);
	}

public:
	void LogTasks() const;
	void ProcessTasks();
	bool TryRunSynchronously(FString* OutError);

public:
	FORCEINLINE void QueueTask(TVoxelUniquePtr<FVoxelTask> TaskPtr)
	{
		checkVoxelSlow(TaskPtr->NumDependencies.Get() == 0);

		if (GVoxelBypassTaskQueue)
		{
			QueueTask_BypassTaskQueue(MoveTemp(TaskPtr));
			return;
		}

		if (FPlatformTLS::GetCurrentThreadId() != ActiveThreadId.Get())
		{
			QueueTask_Async(MoveTemp(TaskPtr));
			return;
		}

		checkVoxelSlow(FPlatformTLS::GetCurrentThreadId() == ActiveThreadId.Get());
		Tasks_Array.Add(MoveTemp(TaskPtr));
	}

private:
	void QueueTask_Async(TVoxelUniquePtr<FVoxelTask> TaskPtr);
	void QueueTask_BypassTaskQueue(TVoxelUniquePtr<FVoxelTask> TaskPtr);

public:
	void AddPendingTask(TVoxelUniquePtr<FVoxelTask> TaskPtr);
	TVoxelUniquePtr<FVoxelTask> RemovePendingTask(FVoxelTask& Task);

private:
	mutable FVoxelCriticalSection PendingTasksCriticalSection;
	TVoxelChunkedSparseArray<TVoxelUniquePtr<FVoxelTask>> PendingTasks_RequiresLock;

public:
	void SetTagNames(TConstVoxelArrayView<FName> TagNames);

private:
	struct FTag
	{
		const FName Name;
		TSharedPtr<FTag> ChildTag;

		explicit FTag(FName Name);
		~FTag();
		UE_NONCOPYABLE(FTag);
	};
	TSharedPtr<FTag> RootTag;

private:
	TVoxelAtomic<uint32> ActiveThreadId = -1;
	TVoxelArray<TVoxelUniquePtr<FVoxelTask>> Tasks_Array;
	TQueue<TVoxelUniquePtr<FVoxelTask>, EQueueMode::Mpsc> Tasks_Queue;

	FVoxelTaskGroup(
		FName Name,
		bool bIsSynchronous,
		const FVoxelTaskPriority& Priority,
		const TSharedRef<FVoxelTaskReferencer>& Referencer,
		const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance);
};