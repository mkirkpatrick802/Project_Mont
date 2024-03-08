// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelRuntime.h"
#include "VoxelActor.h"
#include "VoxelQuery.h"
#include "VoxelGraph.h"
#include "VoxelGraphExecutor.h"
#include "VoxelTerminalGraphInstance.h"
#include "Channel/VoxelChannelManager.h"
#include "VoxelParameterOverridesRuntime.h"
#include "Application/ThrottleManager.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelRuntime);

TSharedRef<FVoxelRuntime> FVoxelRuntime::Create(
	IVoxelRuntimeProvider& RuntimeProvider,
	USceneComponent& RootComponent,
	const FVoxelRuntimeParameters& RuntimeParameters,
	const FVoxelParameterOverridesOwnerPtr& OverridesOwner)
{
	VOXEL_FUNCTION_COUNTER();
	check(OverridesOwner.IsValid());
	ensure(CanBeCreated(true));

	UVoxelGraph* Graph = OverridesOwner->GetGraph();
	if (!Graph)
	{
		// Create a dummy runtime so the IsRuntimeCreated logic in OnProviderChanged
		// doesn't break when assigning another graph
		Graph = LoadObject<UVoxelGraph>(nullptr, TEXT("/Voxel/Default/VG_Empty.VG_Empty"));
	}
	check(Graph);

	LOG_VOXEL(Verbose, "CreateRuntime %s %s", *RootComponent.GetPathName(), *Graph->GetPathName());

	UWorld* World = RootComponent.GetWorld();
	ensure(World);

	const TSharedRef<FVoxelRuntime> Runtime = MakeVoxelShared<FVoxelRuntime>();

	FVoxelRuntimeInfoBase RuntimeInfoBase = FVoxelRuntimeInfoBase::MakeFromWorld(World);
	RuntimeInfoBase.WeakRuntime = Runtime;
	RuntimeInfoBase.WeakWorld = World;
	RuntimeInfoBase.WeakRuntimeProvider = &RuntimeProvider;
	RuntimeInfoBase.WeakRootComponent = &RootComponent;
	RuntimeInfoBase.GraphName = Graph->GetName();
	RuntimeInfoBase.GraphPath = Graph->GetPathName();
	RuntimeInfoBase.InstanceName = RootComponent.GetReadableName() + " (" + World->GetName() + ")";
	RuntimeInfoBase.InstancePath = RootComponent.GetPathName();
	RuntimeInfoBase.Parameters = RuntimeParameters;
	RuntimeInfoBase.LocalToWorld = FVoxelTransformRef::Make(RootComponent);
	Runtime->RuntimeInfo = RuntimeInfoBase.MakeRuntimeInfo_RequiresDestroy();

	const FVoxelTerminalGraphRef TerminalGraphRef(Graph, GVoxelMainTerminalGraphGuid);
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = FVoxelTerminalGraphInstance::Make(
		*Graph,
		TerminalGraphRef,
		Runtime->RuntimeInfo.ToSharedRef(),
		FVoxelParameterOverridesRuntime::Create(OverridesOwner));

	Runtime->GraphExecutor = FVoxelGraphExecutor::Create_GameThread(TerminalGraphRef, TerminalGraphInstance);

	return Runtime;
}

FVoxelRuntime::~FVoxelRuntime()
{
	ensure(RuntimeInfo->IsDestroyed());
}

bool FVoxelRuntime::CanBeCreated(const bool bLog)
{
	if (!GVoxelChannelManager->IsReady(bLog))
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelRuntime::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	const USceneComponent* RootComponent = RuntimeInfo->GetRootComponent();
	LOG_VOXEL(Verbose, "DestroyRuntime %s", RootComponent ? *RootComponent->GetPathName() : TEXT(""));

	GraphExecutor.Reset();
	RuntimeInfo->Destroy();

	for (const TWeakObjectPtr<USceneComponent> WeakComponent : Components)
	{
		USceneComponent* Component = WeakComponent.Get();
		if (!Component)
		{
			// Will be raised if the component is destroyed manually
			// instead of through FVoxelRuntime::DestroyComponent
			ensureVoxelSlow(!RootComponent || GIsGarbageCollecting);
			continue;
		}

		Component->DestroyComponent();
	}

	Components.Empty();
	ComponentPools.Empty();
}

void FVoxelRuntime::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(GraphExecutor))
	{
		return;
	}

	RuntimeInfo->Tick();
	GraphExecutor->Tick(*this);
}

void FVoxelRuntime::AddReferencedObjects(FReferenceCollector& Collector) const
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(GraphExecutor))
	{
		return;
	}

	RuntimeInfo->AddReferencedObjects(Collector);
	GraphExecutor->AddReferencedObjects(Collector);
}

FVoxelOptionalBox FVoxelRuntime::GetBounds() const
{
	return GraphExecutor->GetBounds();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USceneComponent* FVoxelRuntime::CreateComponent(UClass* Class)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	USceneComponent* RootComponent = RuntimeInfo->GetRootComponent();

	if (!ensure(RootComponent) ||
		!ensure(Class) ||
		!ensure(!RuntimeInfo->IsDestroyed()))
	{
		return nullptr;
	}

	TArray<TWeakObjectPtr<USceneComponent>>& Pool = ComponentPools.FindOrAdd(Class);
	while (Pool.Num() > 0)
	{
		if (USceneComponent* Component = Pool.Pop(false).Get())
		{
			return Component;
		}
	}

	AVoxelActor* Actor = Cast<AVoxelActor>(RootComponent->GetOwner());
	if (Actor)
	{
		ensure(!Actor->bDisableModify);
		Actor->bDisableModify = true;
	}

	USceneComponent* Component = NewObject<USceneComponent>(
		RootComponent,
		Class,
		NAME_None,
		RF_Transient |
		RF_DuplicateTransient |
		RF_TextExportTransient);

	Component->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
	Component->SetRelativeTransform(FTransform::Identity);
	Component->RegisterComponent();

	if (Actor)
	{
		ensure(Actor->bDisableModify);
		Actor->bDisableModify = false;
	}

	Components.Add(Component);

	return Component;
}

void FVoxelRuntime::DestroyComponent_ReturnToPoolCalled(USceneComponent& Component)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());
	ensure(Components.Contains(&Component));

	if (!ensure(!RuntimeInfo->IsDestroyed()))
	{
		return;
	}

	Component.SetRelativeTransform(FTransform::Identity);
	ComponentPools.FindOrAdd(Component.GetClass()).Add(&Component);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void FVoxelRuntime::FEditorTicker::Tick()
{
	ensure(!Runtime.RuntimeInfo->IsDestroyed());

	if (FSlateThrottleManager::Get().IsAllowingExpensiveTasks())
	{
		return;
	}

	// If slate is throttling actor tick (eg, when dragging a float property), force tick
	Runtime.Tick();
}
#endif