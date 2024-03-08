// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTransformRef.h"
#include "VoxelRuntimeParameter.h"

class FVoxelRuntime;
class FVoxelRuntimeInfo;
class IVoxelRuntimeProvider;
struct FVoxelTask;
struct FVoxelSubsystem;
struct FVoxelTaskGroupScope;

class VOXELGRAPHCORE_API FVoxelRuntimeInfoBase
{
public:
	TWeakObjectPtr<UWorld> WeakWorld;
	bool bIsGameWorld = false;
	bool bIsPreviewScene = false;
	bool bParallelTasks = false;
	ENetMode NetMode = {};

	TWeakPtr<FVoxelRuntime> WeakRuntime;
	TWeakInterfacePtr<IVoxelRuntimeProvider> WeakRuntimeProvider;
	TWeakObjectPtr<USceneComponent> WeakRootComponent;
	FString GraphName;
	FString GraphPath;
	FString InstanceName;
	FString InstancePath;
	FVoxelRuntimeParameters Parameters;
	FVoxelTransformRef LocalToWorld;

	bool bIsHiddenInEditor = false;
	TSharedPtr<FVoxelDependency> IsHiddenInEditorDependency;

	TVoxelMap<UScriptStruct*, TWeakPtr<FVoxelSubsystem>> WeakSubsystems;
	TVoxelArray<TSharedPtr<FVoxelSubsystem>> SubsystemRefs;

	static FVoxelRuntimeInfoBase MakeFromWorld(UWorld* World);
	static FVoxelRuntimeInfoBase MakePreview();

	FVoxelRuntimeInfoBase& EnableParallelTasks()
	{
		bParallelTasks = true;
		return *this;
	}

	TSharedRef<FVoxelRuntimeInfo> MakeRuntimeInfo_RequiresDestroy();

private:
	FVoxelRuntimeInfoBase() = default;

	bool IsHiddenInEditorImpl(const FVoxelQuery& Query) const;

	template<typename T>
	friend struct TVoxelRuntimeInfo;
};

template<typename T>
struct TVoxelRuntimeInfo
{
public:
	FORCEINLINE FObjectKey GetWorld() const
	{
		return  MakeObjectKey(GetRuntimeInfoBase().WeakWorld);
	}
	FORCEINLINE UWorld* GetWorld_GameThread() const
	{
		ensure(IsInGameThread());
		return GetRuntimeInfoBase().WeakWorld.Get();
	}
	FORCEINLINE bool IsGameWorld() const
	{
		return GetRuntimeInfoBase().bIsGameWorld;
	}
	FORCEINLINE bool IsPreviewScene() const
	{
		return GetRuntimeInfoBase().bIsPreviewScene;
	}
	FORCEINLINE bool ShouldRunTasksInParallel() const
	{
		return GetRuntimeInfoBase().bParallelTasks;
	}
	FORCEINLINE ENetMode GetNetMode() const
	{
		return GetRuntimeInfoBase().NetMode;
	}

public:
	// Will be null if we're in QueryVoxelChannel
	FORCEINLINE TSharedPtr<FVoxelRuntime> GetRuntime() const
	{
		return GetRuntimeInfoBase().WeakRuntime.Pin();
	}
	FORCEINLINE USceneComponent* GetRootComponent() const
	{
		ensure(IsInGameThread());
		return GetRuntimeInfoBase().WeakRootComponent.Get();
	}
	FORCEINLINE const FString& GetGraphName() const
	{
		return GetRuntimeInfoBase().GraphName;
	}
	FORCEINLINE const FString& GetGraphPath() const
	{
		return GetRuntimeInfoBase().GraphPath;
	}
	FORCEINLINE const FString& GetInstanceName() const
	{
		return GetRuntimeInfoBase().InstanceName;
	}
	FORCEINLINE const FString& GetInstancePath() const
	{
		return GetRuntimeInfoBase().InstancePath;
	}
	FORCEINLINE TWeakInterfacePtr<IVoxelRuntimeProvider> GetRuntimeProvider() const
	{
		return GetRuntimeInfoBase().WeakRuntimeProvider;
	}
	FORCEINLINE const FVoxelRuntimeParameters& GetParameters() const
	{
		return GetRuntimeInfoBase().Parameters;
	}
	FORCEINLINE FVoxelTransformRef GetLocalToWorld() const
	{
		return GetRuntimeInfoBase().LocalToWorld;
	}
	FORCEINLINE FVoxelTransformRef GetWorldToLocal() const
	{
		return GetRuntimeInfoBase().LocalToWorld.Inverse();
	}
	FORCEINLINE bool IsHiddenInEditor(const FVoxelQuery& Query) const
	{
		return GetRuntimeInfoBase().IsHiddenInEditorImpl(Query);
	}
	FORCEINLINE const TVoxelMap<UScriptStruct*, TWeakPtr<FVoxelSubsystem>>& GetWeakSubsystems() const
	{
		return GetRuntimeInfoBase().WeakSubsystems;
	}

public:
	template<typename SubsystemType>
	FORCEINLINE TSharedPtr<SubsystemType> FindSubsystem() const
	{
		return StaticCastSharedPtr<SubsystemType>(GetWeakSubsystems().FindRef(StaticStructFast<SubsystemType>()).Pin());
	}
	template<typename RuntimeParameterType, typename = std::enable_if_t<TIsDerivedFrom<RuntimeParameterType, FVoxelRuntimeParameter>::Value>>
	FORCEINLINE TSharedPtr<const RuntimeParameterType> FindParameter() const
	{
		return GetParameters().template Find<RuntimeParameterType>();
	}

private:
	FORCEINLINE const FVoxelRuntimeInfoBase& GetRuntimeInfoBase() const
	{
		return static_cast<const T&>(*this).GetRuntimeInfoRef();
	}
};

class VOXELGRAPHCORE_API FVoxelRuntimeInfo
	: private FVoxelRuntimeInfoBase
	, public TVoxelRuntimeInfo<FVoxelRuntimeInfo>
	, public TSharedFromThis<FVoxelRuntimeInfo>
{
public:
	~FVoxelRuntimeInfo();

	VOXEL_COUNT_INSTANCES();

	FORCEINLINE const FVoxelRuntimeInfo& GetRuntimeInfoRef() const
	{
		return *this;
	}
	FORCEINLINE bool ShouldExitTask() const
	{
		return bDestroyStarted.Get(std::memory_order_relaxed);
	}
	FORCEINLINE bool IsDestroyed() const
	{
		return bIsDestroyed.Get();
	}

	void Destroy();
	void Tick();
	void AddReferencedObjects(FReferenceCollector& Collector);

private:
	explicit FVoxelRuntimeInfo(const FVoxelRuntimeInfoBase& RuntimeInfoBase);

	TVoxelAtomic<bool> bDestroyStarted;
	TVoxelAtomic<bool> bIsDestroyed;

	FVoxelCacheLinePadding Padding;
	mutable FVoxelCounter32 NumActiveTasks;

	friend FVoxelTask;
	friend FVoxelTaskGroupScope;
	friend FVoxelRuntimeInfoBase;
	template<typename>
	friend struct TVoxelRuntimeInfo;
};