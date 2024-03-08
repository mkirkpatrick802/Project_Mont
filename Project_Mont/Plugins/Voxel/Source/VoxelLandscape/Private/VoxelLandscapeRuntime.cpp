// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscapeRuntime.h"
#include "VoxelLandscape.h"
#include "VoxelLandscapeState.h"
#include "VoxelLandscapeBrushManager.h"
#include "Nanite/VoxelLandscapeNaniteMesh.h"
#include "Nanite/VoxelLandscapeNaniteComponent.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "Height/VoxelLandscapeHeightComponent.h"
#include "Volume/VoxelLandscapeVolumeBrush.h"
#include "Volume/VoxelLandscapeVolumeComponent.h"

FVoxelLandscapeRuntime::FVoxelLandscapeRuntime(AVoxelLandscape& Landscape)
	: WeakLandscape(&Landscape)
{
}

void FVoxelLandscapeRuntime::Initialize()
{
	VOXEL_FUNCTION_COUNTER();
	checkVoxelSlow(IsInGameThread());

	const AVoxelLandscape* Landscape = WeakLandscape.Get();
	if (!ensure(Landscape))
	{
		return;
	}

	const TSharedRef<FVoxelLandscapeBrushManager> BrushManager = FVoxelLandscapeBrushManager::Get(Landscape->GetWorld());

	BrushManager->OnChanged2D_GameThread.Add(MakeWeakPtrDelegate(this, [this](const FVoxelLandscapeHeightBrushRuntime& Brush)
	{
		bIsInvalidated = true;

		if (!OldState)
		{
			return;
		}

		const FVoxelBox2D Bounds = Brush.GetBounds(OldState->WorldToVoxel2D);

		if (InvalidatedHeightBounds)
		{
			InvalidatedHeightBounds.GetValue() += Bounds;
		}
		else
		{
			InvalidatedHeightBounds = Bounds;
		}
	}));

	BrushManager->OnChanged_GameThread.Add(MakeWeakPtrDelegate(this, [this](const FVoxelLandscapeVolumeBrushRuntime& Brush)
	{
		bIsInvalidated = true;

		if (!OldState)
		{
			return;
		}

		const FVoxelBox Bounds = Brush.GetBounds(OldState->WorldToVoxel);

		if (InvalidatedVolumeBounds)
		{
			InvalidatedVolumeBounds.GetValue() += Bounds;
		}
		else
		{
			InvalidatedVolumeBounds = Bounds;
		}
	}));

	// Start computing right away
	Tick();
}

void FVoxelLandscapeRuntime::Tick()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const AVoxelLandscape* Landscape = WeakLandscape.Get();
	if (!ensure(Landscape))
	{
		return;
	}

	FVector CameraPosition = FVector(ForceInit);
	ensure(FVoxelUtilities::GetCameraView(Landscape->GetWorld(), CameraPosition));

	if (NewState &&
		NewState->IsReadyToRender())
	{
		Render(*NewState);

		OldState = NewState;
		NewState = nullptr;
	}

	if (OldState)
	{
		const bool bInvalidate = INLINE_LAMBDA
		{
			const FMatrix VoxelToWorld =
				FScaleMatrix(Landscape->VoxelSize) *
				Landscape->ActorToWorld().ToMatrixWithScale();

			if (!OldState->CameraPosition.Equals(CameraPosition, 100.f) ||
				!OldState->VoxelToWorld.Equals(VoxelToWorld))
			{
				return true;
			}

			if (OldState->Material->GetMaterial() != Landscape->Material)
			{
				if (Landscape->Material ||
					OldState->Material != FVoxelMaterialRef::Default())
				{
					OldState.Reset();
					return true;
				}
			}

			if (OldState->TargetVoxelSizeOnScreen != Landscape->TargetVoxelSizeOnScreen ||
				OldState->DetailTextureSize != Landscape->DetailTextureSize ||
				OldState->bEnableNanite != Landscape->bEnableNanite)
			{
				OldState.Reset();
				return true;
			}

			return false;
		};

		if (bInvalidate)
		{
			bIsInvalidated = true;
		}
	}

	if (bIsInvalidated &&
		!NewState)
	{
		NewState = MakeVoxelShared<FVoxelLandscapeState>(
			*Landscape,
			CameraPosition,
			OldState,
			InvalidatedHeightBounds,
			InvalidatedVolumeBounds);

		NewState->Compute();

		bIsInvalidated = false;
		InvalidatedHeightBounds.Reset();
		InvalidatedVolumeBounds.Reset();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeRuntime::Render(const FVoxelLandscapeState& State)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	AVoxelLandscape* Landscape = WeakLandscape.Get();
	if (!ensure(Landscape))
	{
		return;
	}

	{
		for (const auto& It : State.GetChunkKeyToNaniteMesh())
		{
			TWeakObjectPtr<UVoxelLandscapeNaniteComponent>& WeakComponent = ChunkKeyToNaniteComponent.FindOrAdd(It.Key);
			UVoxelLandscapeNaniteComponent* Component = WeakComponent.Get();
			if (!Component)
			{
				Component = Landscape->NewComponent<UVoxelLandscapeNaniteComponent>();
				WeakComponent = Component;
			}
			if (!ensure(Component))
			{
				continue;
			}

			if (Component->GetMesh() == It.Value)
			{
				// Already set
				continue;
			}

			Component->SetMaterial(0, State.Material->GetMaterial());
			Component->SetRelativeScale3D(FVector(1 << It.Value->ChunkKey.LOD) * State.VoxelSize);
			Component->SetRelativeLocation(FVector(It.Key.ChunkKey) * State.ChunkSize * State.VoxelSize);
			Component->SetMesh(It.Value);
		}

		for (auto It = ChunkKeyToNaniteComponent.CreateIterator(); It; ++It)
		{
			if (State.GetChunkKeyToNaniteMesh().Contains(It.Key()))
			{
				continue;
			}

			UStaticMeshComponent* Component = It.Value().Get();
			if (!ensureVoxelSlow(Component))
			{
				It.RemoveCurrent();
				continue;
			}

			Component->SetStaticMesh(nullptr);
			Landscape->RemoveComponent(Component);
			It.RemoveCurrent();
		}
	}

	{
		for (const auto& It : State.GetChunkKeyToHeightMesh())
		{
			TWeakObjectPtr<UVoxelLandscapeHeightComponent>& WeakComponent = ChunkKeyToHeightComponent.FindOrAdd(It.Key);
			UVoxelLandscapeHeightComponent* Component = WeakComponent.Get();
			if (!Component)
			{
				Component = Landscape->NewComponent<UVoxelLandscapeHeightComponent>();
				WeakComponent = Component;
			}
			if (!ensure(Component))
			{
				continue;
			}

			if (Component->GetMesh() == It.Value)
			{
				// Already set
				continue;
			}

			Component->SetRelativeLocation(FVector(It.Key.ChunkKey) * State.ChunkSize * State.VoxelSize);
			Component->SetMesh(It.Value);
		}

		for (auto It = ChunkKeyToHeightComponent.CreateIterator(); It; ++It)
		{
			if (State.GetChunkKeyToHeightMesh().Contains(It.Key()))
			{
				continue;
			}

			UVoxelLandscapeHeightComponent* Component = It.Value().Get();
			if (!ensureVoxelSlow(Component))
			{
				It.RemoveCurrent();
				continue;
			}

			Component->SetMesh(nullptr);
			Landscape->RemoveComponent(Component);
			It.RemoveCurrent();
		}
	}

	{
		for (const auto& It : State.GetChunkKeyToVolumeMesh())
		{
			TWeakObjectPtr<UVoxelLandscapeVolumeComponent>& WeakComponent = ChunkKeyToVolumeComponent.FindOrAdd(It.Key);
			UVoxelLandscapeVolumeComponent* Component = WeakComponent.Get();
			if (!Component)
			{
				Component = Landscape->NewComponent<UVoxelLandscapeVolumeComponent>();
				WeakComponent = Component;
			}
			if (!ensure(Component))
			{
				continue;
			}

			if (Component->GetMesh() == It.Value)
			{
				// Already set
				continue;
			}

			Component->SetRelativeLocation(FVector(It.Key.ChunkKey) * State.ChunkSize * State.VoxelSize);
			Component->SetMesh(It.Value);
		}

		for (auto It = ChunkKeyToVolumeComponent.CreateIterator(); It; ++It)
		{
			if (State.GetChunkKeyToVolumeMesh().Contains(It.Key()))
			{
				continue;
			}

			UVoxelLandscapeVolumeComponent* Component = It.Value().Get();
			if (!ensureVoxelSlow(Component))
			{
				It.RemoveCurrent();
				continue;
			}

			Component->SetMesh(nullptr);
			Landscape->RemoveComponent(Component);
			It.RemoveCurrent();
		}
	}
}