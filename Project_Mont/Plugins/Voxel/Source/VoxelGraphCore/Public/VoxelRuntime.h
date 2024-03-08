// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.h"

struct FVoxelSubsystem;
class FVoxelGraphExecutor;
class FVoxelParameterOverridesOwnerPtr;

class VOXELGRAPHCORE_API FVoxelRuntime
	: public TSharedFromThis<FVoxelRuntime>
	, public TVoxelRuntimeInfo<FVoxelRuntime>
{
public:
	static TSharedRef<FVoxelRuntime> Create(
		IVoxelRuntimeProvider& RuntimeProvider,
		USceneComponent& RootComponent,
		const FVoxelRuntimeParameters& RuntimeParameters,
		const FVoxelParameterOverridesOwnerPtr& OverridesOwner);
	~FVoxelRuntime();

	VOXEL_COUNT_INSTANCES();

	static bool CanBeCreated(bool bLog);

	void Destroy();
	void Tick();
	void AddReferencedObjects(FReferenceCollector& Collector) const;
	FVoxelOptionalBox GetBounds() const;

public:
	FORCEINLINE const FVoxelRuntimeInfo& GetRuntimeInfoRef() const
	{
		return *RuntimeInfo;
	}
	FORCEINLINE const TVoxelSet<TWeakObjectPtr<USceneComponent>>& GetComponents() const
	{
		return Components;
	}

public:
	USceneComponent* CreateComponent(UClass* Class);
	void DestroyComponent_ReturnToPoolCalled(USceneComponent& Component);

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, USceneComponent>::Value>>
	T* CreateComponent()
	{
		return CastChecked<T>(this->CreateComponent(T::StaticClass()), ECastCheckedType::NullAllowed);
	}
	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, USceneComponent>::Value && !std::is_same_v<T, USceneComponent>>>
	void DestroyComponent(TWeakObjectPtr<T>& WeakComponent)
	{
		if (T* Component = WeakComponent.Get())
		{
			Component->ReturnToPool();
			this->DestroyComponent_ReturnToPoolCalled(*Component);
		}
		WeakComponent = {};
	}

private:
	TSharedPtr<FVoxelRuntimeInfo> RuntimeInfo;
	TSharedPtr<FVoxelGraphExecutor> GraphExecutor;

	TVoxelSet<TWeakObjectPtr<USceneComponent>> Components;
	TVoxelMap<UClass*, TArray<TWeakObjectPtr<USceneComponent>>> ComponentPools;

#if WITH_EDITOR
	struct FEditorTicker : public FVoxelTicker
	{
		FVoxelRuntime& Runtime;

		explicit FEditorTicker(FVoxelRuntime& Runtime)
			: Runtime(Runtime)
		{
		}

		virtual void Tick() override;
	};
	const FEditorTicker EditorTicker = FEditorTicker(*this);
#endif
};