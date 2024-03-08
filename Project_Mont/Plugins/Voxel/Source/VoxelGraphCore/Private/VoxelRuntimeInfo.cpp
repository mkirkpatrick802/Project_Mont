// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelRuntimeInfo.h"
#include "VoxelQuery.h"
#include "VoxelSubsystem.h"
#include "VoxelDependencyTracker.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelRuntimeInfo);

FVoxelRuntimeInfoBase FVoxelRuntimeInfoBase::MakeFromWorld(UWorld* World)
{
	ensure(IsInGameThread());

	if (!ensure(World))
	{
		return {};
	}

	FVoxelRuntimeInfoBase Result;
	Result.WeakWorld = World;
	Result.bIsGameWorld = World->IsGameWorld();
	Result.bIsPreviewScene = World->WorldType == EWorldType::EditorPreview;
	Result.NetMode = World->GetNetMode();
	return Result;
}

FVoxelRuntimeInfoBase FVoxelRuntimeInfoBase::MakePreview()
{
	FVoxelRuntimeInfoBase Result;
	Result.bIsPreviewScene = true;
	return Result;
}

TSharedRef<FVoxelRuntimeInfo> FVoxelRuntimeInfoBase::MakeRuntimeInfo_RequiresDestroy()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(WeakSubsystems.Num() == 0);

	ensure(!IsHiddenInEditorDependency);
	IsHiddenInEditorDependency = FVoxelDependency::Create(
		STATIC_FNAME("IsHiddenInEditor"),
		WeakRuntimeProvider.IsValid() ? WeakRuntimeProvider.GetObject()->GetFName() : FName());

	static const TArray<UScriptStruct*> SubsystemStructs = GetDerivedStructs<FVoxelSubsystem>();

	const TSharedRef<FVoxelRuntimeInfo> RuntimeInfoWithoutSubsystems = MakeVoxelShareable(new (GVoxelMemory) FVoxelRuntimeInfo(*this));
	for (UScriptStruct* Struct : SubsystemStructs)
	{
		const TSharedRef<FVoxelSubsystem> Subsystem = MakeSharedStruct<FVoxelSubsystem>(Struct);
		Subsystem->RuntimeInfo = RuntimeInfoWithoutSubsystems;
		if (!Subsystem->ShouldCreateSubsystem())
		{
			continue;
		}

		WeakSubsystems.Add_CheckNew(Struct, Subsystem);
		SubsystemRefs.Add(Subsystem);
	}
	RuntimeInfoWithoutSubsystems->Destroy();

	const TSharedRef<FVoxelRuntimeInfo> RuntimeInfo = MakeVoxelShareable(new (GVoxelMemory) FVoxelRuntimeInfo(*this));
	RuntimeInfo->Tick();

	for (const TSharedPtr<FVoxelSubsystem>& Subsystem : SubsystemRefs)
	{
		Subsystem->RuntimeInfo = RuntimeInfo;
		Subsystem->Create();
	}

	return RuntimeInfo;
}

bool FVoxelRuntimeInfoBase::IsHiddenInEditorImpl(const FVoxelQuery& Query) const
{
	Query.GetDependencyTracker().AddDependency(IsHiddenInEditorDependency.ToSharedRef());
	return bIsHiddenInEditor;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimeInfo::FVoxelRuntimeInfo(const FVoxelRuntimeInfoBase& RuntimeInfoBase)
	: FVoxelRuntimeInfoBase(RuntimeInfoBase)
{
}

FVoxelRuntimeInfo::~FVoxelRuntimeInfo()
{
	ensure(IsDestroyed());
	ensure(NumActiveTasks.Get() == 0);
}

void FVoxelRuntimeInfo::Destroy()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!IsDestroyed());

	bDestroyStarted.Set(true);

	if (NumActiveTasks.Get() > 0)
	{
		VOXEL_SCOPE_COUNTER("Wait");

		double NextLogTime = FPlatformTime::Seconds() + 0.5;

		while (NumActiveTasks.Get() > 0)
		{
			if (NextLogTime < FPlatformTime::Seconds())
			{
				NextLogTime += 0.5;
				LOG_VOXEL(Log, "Destroy: Waiting for %d tasks", NumActiveTasks.Get());
			}

			FPlatformProcess::YieldThread();
		}
	}

	// Not always true, see FVoxelTaskGroupScope::Initialize
	// check(NumActiveTasks.GetValue() == 0);

	for (const TSharedPtr<FVoxelSubsystem>& Subsystem : SubsystemRefs)
	{
		Subsystem->Destroy();
	}

	// Make sure to empty, otherwise the subsystems RuntimeInfo will keep themselves alive
	SubsystemRefs.Empty();

	ensure(!IsDestroyed());
	bIsDestroyed.Set(true);
}

void FVoxelRuntimeInfo::Tick()
{
	VOXEL_FUNCTION_COUNTER();

#if WITH_EDITOR
	if (const AActor* Actor = Cast<AActor>(WeakRuntimeProvider.GetObject()))
	{
		const bool bNewIsHiddenInEditor = Actor->IsHiddenEd();
		if (bIsHiddenInEditor != bNewIsHiddenInEditor)
		{
			bIsHiddenInEditor = bNewIsHiddenInEditor;
			IsHiddenInEditorDependency->Invalidate();
		}
	}
#endif

	for (const TSharedPtr<FVoxelSubsystem>& Subsystem : SubsystemRefs)
	{
		Subsystem->Tick();
	}
}

void FVoxelRuntimeInfo::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();

	for (const TSharedPtr<FVoxelSubsystem>& Subsystem : SubsystemRefs)
	{
		Subsystem->AddReferencedObjects(Collector);
	}
}