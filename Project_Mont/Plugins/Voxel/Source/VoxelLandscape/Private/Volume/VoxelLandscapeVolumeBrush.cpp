// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/VoxelLandscapeVolumeBrush.h"
#include "Volume/VoxelLandscapeVolumeProvider.h"
#include "VoxelLandscapeBrushManager.h"

TSharedPtr<FVoxelLandscapeVolumeBrushRuntime> FVoxelLandscapeVolumeBrushRuntime::Create(const AVoxelLandscapeVolumeBrush& Brush)
{
	VOXEL_FUNCTION_COUNTER();

	if (!Brush.Volume.IsValid())
	{
		return nullptr;
	}

	const TSharedRef<FVoxelLandscapeVolumeBrushRuntime> Runtime = MakeVoxelShareable(new (GVoxelMemory) FVoxelLandscapeVolumeBrushRuntime(Brush));
	if (!Runtime->VolumeProvider->TryInitialize())
	{
		return nullptr;
	}

	Runtime->RegisterBrush();
	return Runtime;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelBox FVoxelLandscapeVolumeBrushRuntime::GetBounds(const FMatrix& WorldToVoxel) const
{
	ensure(PrivateLocalBounds.IsValidAndNotEmpty());

	return PrivateLocalBounds
		.TransformBy(LocalToWorld)
		.Extend(Smoothness)
		.TransformBy(WorldToVoxel);
}

void FVoxelLandscapeVolumeBrushRuntime::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	if (BrushIndex == -1)
	{
		return;
	}

	FVoxelLandscapeBrushManager::Get(World)->UnregisterVolumeBrush(BrushIndex);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelLandscapeVolumeBrushRuntime::FVoxelLandscapeVolumeBrushRuntime(const AVoxelLandscapeVolumeBrush& Brush)
	: World(Brush.GetWorld())
	, LocalToWorld(Brush.ActorToWorld())
	, BlendMode(Brush.BlendMode)
	, Smoothness(Brush.Smoothness)
	, Priority(Brush.Priority)
	, VolumeProvider(Brush.Volume.MakeSharedCopy())
{
}

void FVoxelLandscapeVolumeBrushRuntime::RegisterBrush()
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelBox NewBounds = VolumeProvider->GetBounds();
	if (!ensure(NewBounds.IsValidAndNotEmpty()))
	{
		return;
	}

	// TODO Scale?
	PrivateLocalBounds = NewBounds;

	ensure(BrushIndex == -1);
	BrushIndex = FVoxelLandscapeBrushManager::Get(World)->RegisterVolumeBrush(SharedThis(this));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR

void AVoxelLandscapeVolumeBrush::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_OWN_MEMBER_NAME(BlendMode) ||
		PropertyChangedEvent.GetPropertyName() == GET_OWN_MEMBER_NAME(Smoothness) ||
		PropertyChangedEvent.GetPropertyName() == GET_OWN_MEMBER_NAME(Priority))
	{
		QueueRecreate();
	}
}
#endif

void AVoxelLandscapeVolumeBrush::NotifyTransformChanged()
{
	QueueRecreate();
}

bool AVoxelLandscapeVolumeBrush::ShouldDestroyWhenHidden() const
{
	return true;
}

FVoxelBox AVoxelLandscapeVolumeBrush::GetLocalBounds() const
{
	VOXEL_FUNCTION_COUNTER();

	if (!Volume)
	{
		return {};
	}

	const TSharedRef<FVoxelLandscapeVolumeProvider> VolumeCopy = Volume.MakeSharedCopy();
	if (!VolumeCopy->TryInitialize())
	{
		return {};
	}

	return VolumeCopy->GetBounds();
}

TSharedPtr<IVoxelActorRuntime> AVoxelLandscapeVolumeBrush::CreateNewRuntime()
{
	return FVoxelLandscapeVolumeBrushRuntime::Create(*this);
}