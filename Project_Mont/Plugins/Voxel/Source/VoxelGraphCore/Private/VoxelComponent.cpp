// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelComponent.h"
#include "VoxelGraph.h"
#include "VoxelRuntime.h"
#include "VoxelParameterContainer_DEPRECATED.h"

UVoxelComponent::UVoxelComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bWantsOnUpdateTransform = true;
	bTickInEditor = true;
}

UVoxelComponent::~UVoxelComponent()
{
	ensure(!Runtime.IsValid());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelComponent::OnRegister()
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnRegister();

	if (!IsRuntimeCreated())
	{
		QueueCreateRuntime();
	}
}

void UVoxelComponent::OnUnregister()
{
	VOXEL_FUNCTION_COUNTER();

	if (IsRuntimeCreated())
	{
		DestroyRuntime();
	}

	Super::OnUnregister();
}

void UVoxelComponent::PostLoad()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostLoad();

	if (UVoxelGraph* OldGraph = Graph_DEPRECATED.LoadSynchronous())
	{
		ensure(!Graph);
		Graph = OldGraph;
	}

	if (ParameterCollection_DEPRECATED.Parameters.Num() > 0)
	{
		ParameterCollection_DEPRECATED.MigrateTo(*this);
	}

	if (ParameterContainer_DEPRECATED)
	{
		ensure(!Graph);
		Graph = CastEnsured<UVoxelGraph>(ParameterContainer_DEPRECATED->Provider);

		ParameterContainer_DEPRECATED->MigrateTo(ParameterOverrides);
	}

	FixupParameterOverrides();
}

void UVoxelComponent::PostInitProperties()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostInitProperties();

	FixupParameterOverrides();
}

void UVoxelComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	VOXEL_FUNCTION_COUNTER();

	// We don't want to tick the BP in preview
	if (GetWorld()->IsGameWorld())
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}

	if (bRuntimeCreateQueued &&
		FVoxelRuntime::CanBeCreated(false))
	{
		CreateRuntime();
	}

	if (bRuntimeRecreateQueued &&
		FVoxelRuntime::CanBeCreated(false))
	{
		bRuntimeRecreateQueued = false;

		DestroyRuntime();
		CreateRuntime();
	}

	if (Runtime)
	{
		Runtime->Tick();
	}
}

void UVoxelComponent::UpdateBounds()
{
	VOXEL_FUNCTION_COUNTER();

	Super::UpdateBounds();
	FVoxelTransformRef::NotifyTransformChanged(*this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelComponent::PostEditUndo()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditUndo();

	FixupParameterOverrides();

	if (IsValid(this))
	{
		if (!IsRuntimeCreated())
		{
			QueueCreateRuntime();
		}
	}
	else
	{
		if (IsRuntimeCreated())
		{
			DestroyRuntime();
		}
	}
}

void UVoxelComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(Graph_NewProperty))
	{
		NotifyGraphChanged();

		if (IsRuntimeCreated())
		{
			QueueRecreate();
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelComponent::ShouldForceEnableOverride(const FVoxelParameterPath& Path) const
{
	return true;
}

FVoxelParameterOverrides& UVoxelComponent::GetParameterOverrides()
{
	return ParameterOverrides;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelComponent::QueueCreateRuntime()
{
	VOXEL_FUNCTION_COUNTER();

	if (FVoxelRuntime::CanBeCreated(false))
	{
		CreateRuntime();
	}
	else
	{
		bRuntimeCreateQueued = true;
	}
}

void UVoxelComponent::CreateRuntime()
{
	VOXEL_FUNCTION_COUNTER();

	if (!FVoxelRuntime::CanBeCreated(true))
	{
		VOXEL_MESSAGE(Error, "{0}: Cannot create runtime: not ready. See log for more info", this);
		return;
	}

	bRuntimeCreateQueued = false;

	if (IsRuntimeCreated())
	{
		return;
	}

	Runtime = FVoxelRuntime::Create(
		*this,
		*this,
		{},
		this);

	OnRuntimeCreated.Broadcast();
}

void UVoxelComponent::DestroyRuntime()
{
	VOXEL_FUNCTION_COUNTER();

	// Clear RuntimeRecreate queue
	bRuntimeRecreateQueued = false;

	if (!IsRuntimeCreated())
	{
		return;
	}

	OnRuntimeDestroyed.Broadcast();

	Runtime->Destroy();
	Runtime.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelComponent::SetGraph(UVoxelGraph* NewGraph)
{
	if (Graph == NewGraph)
	{
		return;
	}
	Graph = NewGraph;

	NotifyGraphChanged();

	if (IsRuntimeCreated())
	{
		QueueRecreate();
	}
}