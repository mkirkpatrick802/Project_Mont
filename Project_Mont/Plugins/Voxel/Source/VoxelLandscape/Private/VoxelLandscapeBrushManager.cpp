// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscapeBrushManager.h"
#include "Height/VoxelLandscapeHeightBrush.h"
#include "Volume/VoxelLandscapeVolumeBrush.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELLANDSCAPE_API, bool, GVoxelLandscapeShowBrushBounds, false,
	"voxel.landscape.ShowBrushBounds",
	"");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelLandscapeBrushes::FVoxelLandscapeBrushes(
	const FMatrix& WorldToVoxel,
	const TVoxelSparseArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>>& InHeightBrushes,
	const TVoxelSparseArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>>& InVolumeBrushes)
{
	VOXEL_SCOPE_COUNTER_FORMAT("Build Brushes Num Heights = %d Num Volume = %d",
		InHeightBrushes.Num(),
		InVolumeBrushes.Num());

	HeightBrushes.Reserve(InHeightBrushes.Num());
	VolumeBrushes.Reserve(InVolumeBrushes.Num());

	for (const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& HeightBrush : InHeightBrushes)
	{
		HeightBrushes.Add(HeightBrush);
	}
	for (const TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>& VolumeBrush : InVolumeBrushes)
	{
		VolumeBrushes.Add(VolumeBrush);
	}

	const FTransform2d WorldToVoxel2D = FVoxelUtilities::MakeTransform2(WorldToVoxel);

	{
		TVoxelArray<FVoxelAABBTree2D::FElement> HeightElements;
		HeightElements.Reserve(HeightBrushes.Num());

		for (int32 Index = 0; Index < HeightBrushes.Num(); Index++)
		{
			const FVoxelLandscapeHeightBrushRuntime& Brush = *HeightBrushes[Index];

			FPayload Payload;
			Payload.bIsHeight = true;
			Payload.Index = Index;

			FVoxelAABBTree2D::FElement Element;
			Element.Bounds = Brush.GetBounds(WorldToVoxel2D);
			Element.Payload = ReinterpretCastRef<int32>(Payload);
			HeightElements.Add(Element);
		}

		HeightTree.Initialize(MoveTemp(HeightElements));
	}

	{
		TVoxelArray<FVoxelAABBTree::FElement> VolumeElements;
		VolumeElements.Reserve(VolumeBrushes.Num());

		for (int32 Index = 0; Index < VolumeBrushes.Num(); Index++)
		{
			const FVoxelLandscapeVolumeBrushRuntime& Brush = *VolumeBrushes[Index];

			FPayload Payload;
			Payload.bIsHeight = false;
			Payload.Index = Index;

			FVoxelAABBTree::FElement Element;
			Element.Bounds = Brush.GetBounds(WorldToVoxel);
			Element.Payload = ReinterpretCastRef<int32>(Payload);
			VolumeElements.Add(Element);
		}

		VolumeTree.Initialize(MoveTemp(VolumeElements));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelLandscapeBrushes::IsEmpty() const
{
	return
		HeightBrushes.Num() == 0 &&
		VolumeBrushes.Num() == 0;
}

FVoxelBox FVoxelLandscapeBrushes::GetBounds() const
{
	ensure(!IsEmpty());

	FVoxelBox Bounds = FVoxelBox::InvertedInfinite;

	if (HeightBrushes.Num() > 0)
	{
		// TODO proper bounds
		Bounds += FVector(HeightTree.GetBounds().Min, 0);
		Bounds += FVector(HeightTree.GetBounds().Max, 32000);
	}

	if (VolumeBrushes.Num() > 0)
	{
		Bounds += VolumeTree.GetBounds();
	}

	return Bounds;
}

bool FVoxelLandscapeBrushes::Intersect(const FVoxelBox& Bounds) const
{
	return
		HeightTree.Intersect(FVoxelBox2D(Bounds)) ||
		VolumeTree.Intersect(Bounds);
}

bool FVoxelLandscapeBrushes::HasVolumeBrushes(const FVoxelBox& Bounds) const
{
	bool bHasVolumeBrushes = false;
	VolumeTree.TraverseBounds(Bounds, [&](const int32 PackedPayload)
	{
		if (!ReinterpretCastRef<FPayload>(PackedPayload).bIsHeight)
		{
			bHasVolumeBrushes = true;
		}
	});
	return bHasVolumeBrushes;
}

TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> FVoxelLandscapeBrushes::GetHeightBrushes(const FVoxelBox2D& Bounds) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<TSharedPtr<FVoxelLandscapeHeightBrushRuntime>> Result;
	Result.Reserve(128);

	HeightTree.TraverseBounds(Bounds, [&](const int32 PackedPayload)
	{
		const FPayload Payload = ReinterpretCastRef<FPayload>(PackedPayload);
		if (!Payload.bIsHeight)
		{
			return;
		}

		Result.Add(HeightBrushes[Payload.Index]);
	});

	Result.Sort([](const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& BrushA, const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& BrushB)
	{
		return BrushA->Priority < BrushB->Priority;
	});

	return Result;
}

TVoxelArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> FVoxelLandscapeBrushes::GetVolumeBrushes(const FVoxelBox& Bounds) const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>> Result;
	Result.Reserve(128);

	VolumeTree.TraverseBounds(Bounds, [&](const int32 PackedPayload)
	{
		const FPayload Payload = ReinterpretCastRef<FPayload>(PackedPayload);
		if (Payload.bIsHeight)
		{
			return;
		}

		Result.Add(VolumeBrushes[Payload.Index]);
	});

	Result.Sort([](const TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>& BrushA, const TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>& BrushB)
	{
		return BrushA->Priority < BrushB->Priority;
	});

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FVoxelLandscapeBrushManager::RegisterHeightBrush(const TSharedRef<FVoxelLandscapeHeightBrushRuntime>& Brush)
{
	check(IsInGameThreadFast());

	PendingActions_GameThread.Add(FAction
	{
		EType::AddHeight,
		-1,
		MakeSharedVoidRef(Brush)
	});

	OnChanged2D_GameThread.Broadcast(*Brush);

	return HeightBrushes_GameThread.Add(Brush);
}

void FVoxelLandscapeBrushManager::UnregisterHeightBrush(const int32 BrushIndex)
{
	check(IsInGameThreadFast());

	PendingActions_GameThread.Add(FAction
	{
		EType::RemoveHeight,
		BrushIndex,
		nullptr
	});

	OnChanged2D_GameThread.Broadcast(*HeightBrushes_GameThread[BrushIndex]);

	HeightBrushes_GameThread.RemoveAt(BrushIndex);
}

int32 FVoxelLandscapeBrushManager::RegisterVolumeBrush(const TSharedRef<FVoxelLandscapeVolumeBrushRuntime>& Brush)
{
	check(IsInGameThreadFast());

	PendingActions_GameThread.Add(FAction
	{
		EType::AddVolume,
		-1,
		MakeSharedVoidRef(Brush)
	});

	OnChanged_GameThread.Broadcast(*Brush);

	return VolumeBrushes_GameThread.Add(Brush);
}

void FVoxelLandscapeBrushManager::UnregisterVolumeBrush(const int32 BrushIndex)
{
	check(IsInGameThreadFast());

	PendingActions_GameThread.Add(FAction
	{
		EType::RemoveVolume,
		BrushIndex,
		nullptr
	});

	OnChanged_GameThread.Broadcast(*VolumeBrushes_GameThread[BrushIndex]);

	VolumeBrushes_GameThread.RemoveAt(BrushIndex);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFuture<FVoxelLandscapeBrushes> FVoxelLandscapeBrushManager::GetBrushes(const FMatrix& WorldToVoxel)
{
	check(IsInGameThread());

	return AsyncBackgroundTask(MakeStrongPtrLambda(this, [=, PendingActions = MoveTemp(PendingActions_GameThread)]
	{
		VOXEL_FUNCTION_COUNTER();
		VOXEL_SCOPE_LOCK(AsyncData.CriticalSection);

		for (const FAction& Action : PendingActions)
		{
			switch (Action.Type)
			{
			default: check(false);
			case EType::AddHeight:
			{
				AsyncData.HeightBrushes.Add(ReinterpretCastRef<TSharedRef<FVoxelLandscapeHeightBrushRuntime>>(Action.Brush));
			}
			break;
			case EType::AddVolume:
			{
				AsyncData.VolumeBrushes.Add(ReinterpretCastRef<TSharedRef<FVoxelLandscapeVolumeBrushRuntime>>(Action.Brush));
			}
			break;
			case EType::RemoveHeight:
			{
				AsyncData.HeightBrushes.RemoveAt(Action.BrushIndex);
			}
			break;
			case EType::RemoveVolume:
			{
				AsyncData.VolumeBrushes.RemoveAt(Action.BrushIndex);
			}
			break;
			}
		}

		return MakeVoxelShared<FVoxelLandscapeBrushes>(
			WorldToVoxel,
			AsyncData.HeightBrushes,
			AsyncData.VolumeBrushes);
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeBrushManager::Tick()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (!GVoxelLandscapeShowBrushBounds)
	{
		return;
	}

	for (const TSharedPtr<FVoxelLandscapeHeightBrushRuntime>& Brush : HeightBrushes_GameThread)
	{
		const FVoxelBox2D Bounds = Brush->GetBounds(Brush->LocalToWorld.Inverse());

		FVoxelDebugDrawer(GetWorld())
			.OneFrame()
			.Color(FLinearColor::Blue)
			.DrawBox(Bounds.ToBox3D(0, 32000), Brush->LocalToWorld.To3DMatrix());
	}

	for (const TSharedPtr<FVoxelLandscapeVolumeBrushRuntime>& Brush : VolumeBrushes_GameThread)
	{
		const FVoxelBox Bounds = Brush->GetBounds(Brush->LocalToWorld.ToInverseMatrixWithScale());

		FVoxelDebugDrawer(GetWorld())
			.OneFrame()
			.Color(FLinearColor::Red)
			.DrawBox(Bounds, Brush->LocalToWorld);
	}
}