// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelActor.h"
#include "VoxelGraph.h"
#include "VoxelRuntime.h"
#include "Point/VoxelPointStorage.h"
#include "Sculpt/VoxelSculptStorage.h"
#include "Sculpt/VoxelExecNode_EditSculptSurface.h"
#include "VoxelParameterContainer_DEPRECATED.h"
#include "Engine/Texture2D.h"
#include "Components/BillboardComponent.h"

UVoxelActorRootComponent::UVoxelActorRootComponent()
{
	bWantsOnUpdateTransform = true;
}

void UVoxelActorRootComponent::UpdateBounds()
{
	VOXEL_FUNCTION_COUNTER();

	Super::UpdateBounds();
	FVoxelTransformRef::NotifyTransformChanged(*this);
}

AVoxelActor::AVoxelActor()
{
	RootComponent = CreateDefaultSubobject<UVoxelActorRootComponent>("Root");
	PointStorageComponent = CreateDefaultSubobject<UVoxelPointStorage>("VoxelPointStorage");
	SculptStorageComponent = CreateDefaultSubobject<UVoxelSculptStorage>("VoxelSculptStorage");

	if (PointStorageComponent)
	{
		const TSharedRef<FVoxelRuntimeParameter_PointStorage> Parameter = MakeVoxelShared<FVoxelRuntimeParameter_PointStorage>();
		Parameter->Data = PointStorageComponent->GetData();
		DefaultRuntimeParameters.Add(Parameter);
	}

	if (SculptStorageComponent)
	{
		const TSharedRef<FVoxelRuntimeParameter_SculptStorage> Parameter = MakeVoxelShared<FVoxelRuntimeParameter_SculptStorage>();
		Parameter->Data = SculptStorageComponent->GetData();
		DefaultRuntimeParameters.Add(Parameter);
	}

	DefaultRuntimeParameters.Add(MakeVoxelShared<FVoxelRuntimeParameter_EditSculptSurface>());

	PrimaryActorTick.bCanEverTick = true;

#if WITH_EDITORONLY_DATA
	bIsSpatiallyLoaded = false;
#endif

#if WITH_EDITOR
	if (UBillboardComponent* SpriteComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite")))
	{
		static ConstructorHelpers::FObjectFinder<UTexture2D> SpriteFinder(TEXT("/Voxel/EditorAssets/T_VoxelIcon"));
		SpriteComponent->Sprite = SpriteFinder.Object;
		SpriteComponent->SetRelativeScale3D(FVector(0.5f));
		SpriteComponent->bHiddenInGame = true;
		SpriteComponent->bIsScreenSizeScaled = true;
		SpriteComponent->SpriteInfo.Category = TEXT("Voxel Actor");
		SpriteComponent->SpriteInfo.DisplayName = FText::FromString("Voxel Actor");
		SpriteComponent->SetupAttachment(RootComponent);
		SpriteComponent->bReceivesDecals = false;
	}
#endif
}

AVoxelActor::~AVoxelActor()
{
	ensure(!Runtime.IsValid());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AVoxelActor::BeginPlay()
{
	VOXEL_FUNCTION_COUNTER();

	Super::BeginPlay();

	if (bCreateRuntimeOnBeginPlay &&
		!IsRuntimeCreated())
	{
		QueueCreateRuntime();
	}
}

void AVoxelActor::BeginDestroy()
{
	VOXEL_FUNCTION_COUNTER();

	if (IsRuntimeCreated())
	{
		DestroyRuntime();
	}

	Super::BeginDestroy();
}

void AVoxelActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	VOXEL_FUNCTION_COUNTER();

	// In the editor, Destroyed is called but EndPlay isn't

	if (IsRuntimeCreated())
	{
		DestroyRuntime();
	}

	Super::EndPlay(EndPlayReason);
}

void AVoxelActor::Destroyed()
{
	VOXEL_FUNCTION_COUNTER();

	if (IsRuntimeCreated())
	{
		DestroyRuntime();
	}

	Super::Destroyed();
}

void AVoxelActor::OnConstruction(const FTransform& Transform)
{
	VOXEL_FUNCTION_COUNTER();

	Super::OnConstruction(Transform);

#if WITH_EDITOR
	if (bCreateRuntimeOnConstruction_EditorOnly &&
		!IsRuntimeCreated() &&
		GetWorld() &&
		!GetWorld()->IsGameWorld() &&
		!IsTemplate() &&
		!IsRunningCommandlet())
	{
		// Force queue to avoid creating runtime twice on map load
		bRuntimeCreateQueued = true;
	}
#endif
}

void AVoxelActor::PostLoad()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostLoad();

	if (!PointStorageComponent)
	{
		PointStorageComponent = NewObject<UVoxelPointStorage>(this, "VoxelPointStorage");
	}
	if (!SculptStorageComponent)
	{
		SculptStorageComponent = NewObject<UVoxelSculptStorage>(this, "VoxelSculptStorage");
	}

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

void AVoxelActor::PostEditImport()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditImport();

	FixupParameterOverrides();

	if (IsRuntimeCreated())
	{
		QueueRecreate();
	}
}

void AVoxelActor::PostInitProperties()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostInitProperties();

	FixupParameterOverrides();
}

void AVoxelActor::PostCDOContruct()
{
	Super::PostCDOContruct();

	FVoxelUtilities::ForceLoadDeprecatedSubobject<UVoxelParameterContainer_DEPRECATED>(this, "ParameterContainerComponent");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
bool AVoxelActor::Modify(const bool bAlwaysMarkDirty)
{
	if (bDisableModify)
	{
		return false;
	}

	return Super::Modify(bAlwaysMarkDirty);
}

void AVoxelActor::PostEditUndo()
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

void AVoxelActor::PreEditChange(FProperty* PropertyThatWillChange)
{
	// Temporarily remove runtime components to avoid expensive re-registration
	if (Runtime)
	{
		for (const TWeakObjectPtr<USceneComponent>& Component : Runtime->GetComponents())
		{
			ensure(Component.IsValid());
			ensure(ConstCast(GetComponents()).Remove(Component.Get()));
		}
	}

	Super::PreEditChange(PropertyThatWillChange);

	if (Runtime)
	{
		for (const TWeakObjectPtr<USceneComponent>& Component : Runtime->GetComponents())
		{
			ensure(Component.IsValid());
			ConstCast(GetComponents()).Add(Component.Get());
		}
	}
}

void AVoxelActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// Temporarily remove runtime components to avoid expensive re-registration
	if (Runtime)
	{
		for (const TWeakObjectPtr<USceneComponent>& Component : Runtime->GetComponents())
		{
			ensure(Component.IsValid());
			ensure(ConstCast(GetComponents()).Remove(Component.Get()));
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(Graph_NewProperty))
	{
		NotifyGraphChanged();

		if (IsRuntimeCreated())
		{
			QueueRecreate();
		}
	}

	if (Runtime)
	{
		for (const TWeakObjectPtr<USceneComponent>& Component : Runtime->GetComponents())
		{
			ensure(Component.IsValid());
			ConstCast(GetComponents()).Add(Component.Get());
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AVoxelActor::Tick(const float DeltaTime)
{
	VOXEL_FUNCTION_COUNTER();

	// We don't want to tick the BP in preview
	if (GetWorld()->IsGameWorld())
	{
		Super::Tick(DeltaTime);
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool AVoxelActor::ShouldForceEnableOverride(const FVoxelParameterPath& Path) const
{
	return true;
}

FVoxelParameterOverrides& AVoxelActor::GetParameterOverrides()
{
	return ParameterOverrides;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AVoxelActor::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	const TSharedPtr<FVoxelRuntime> Runtime = CastChecked<AVoxelActor>(InThis)->Runtime;
	if (!Runtime)
	{
		return;
	}

	Runtime->AddReferencedObjects(Collector);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimeParameters AVoxelActor::GetRuntimeParameters() const
{
	return DefaultRuntimeParameters;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AVoxelActor::QueueCreateRuntime()
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

void AVoxelActor::CreateRuntime()
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

	USceneComponent* Component = GetRootComponent();
	if (!ensure(Component))
	{
		return;
	}

	Runtime = FVoxelRuntime::Create(
		*this,
		*Component,
		GetRuntimeParameters(),
		this);

	OnRuntimeCreated.Broadcast();
}

void AVoxelActor::DestroyRuntime()
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

void AVoxelActor::SetGraph(UVoxelGraph* NewGraph)
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