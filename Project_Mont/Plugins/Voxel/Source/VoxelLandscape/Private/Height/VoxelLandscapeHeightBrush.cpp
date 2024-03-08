// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Height/VoxelLandscapeHeightBrush.h"
#include "Height/VoxelLandscapeHeightProvider.h"
#include "VoxelLandscapeState.h"
#include "VoxelLandscapeBrushManager.h"

TSharedPtr<FVoxelLandscapeHeightBrushRuntime> FVoxelLandscapeHeightBrushRuntime::Create(const AVoxelLandscapeHeightBrush& Brush)
{
	VOXEL_FUNCTION_COUNTER();

	if (!Brush.Height.IsValid())
	{
		return nullptr;
	}

	const TSharedRef<FVoxelLandscapeHeightBrushRuntime> Runtime = MakeVoxelShareable(new (GVoxelMemory) FVoxelLandscapeHeightBrushRuntime(Brush));
	if (!Runtime->HeightProvider->TryInitialize())
	{
		return nullptr;
	}

	Runtime->RegisterBrush();
	return Runtime;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelBox2D FVoxelLandscapeHeightBrushRuntime::GetBounds(const FTransform2d& WorldToVoxel) const
{
	ensure(PrivateLocalBounds.IsValidAndNotEmpty());

	return PrivateLocalBounds
		.TransformBy(LocalToWorld)
		.Extend(Smoothness)
		.TransformBy(WorldToVoxel);
}

FVoxelLandscapeHeightQuery FVoxelLandscapeHeightBrushRuntime::MakeQuery(
	const FVoxelLandscapeState& State,
	const FVector2D& Start,
	const double Step,
	const FIntPoint& Size,
	const TVoxelArrayView<float> Heights) const
{
	VOXEL_FUNCTION_COUNTER();
	check(Heights.Num() == Size.X * Size.Y);

	const FVoxelBox2D VoxelBounds = GetBounds(State.WorldToVoxel2D);
	const FVoxelBox2D LocalBounds = VoxelBounds.ShiftBy(-Start).Scale(1. / Step);

	const FTransform2d IndexToBrush =
		FTransform2d(FScale2d(Step)) *
		FTransform2d(FVector2D(Start.X, Start.Y)) *
		State.WorldToVoxel2D.Inverse() *
		LocalToWorld.Inverse();

	FVoxelLandscapeHeightQuery Query;
	Query.Heights = Heights;
	Query.Span.Min.X = FMath::Max(0, FMath::FloorToInt(LocalBounds.Min.X));
	Query.Span.Min.Y = FMath::Max(0, FMath::FloorToInt(LocalBounds.Min.Y));
	Query.Span.Max.X = FMath::Min(Size.X, FMath::CeilToInt(LocalBounds.Max.X));
	Query.Span.Max.Y = FMath::Min(Size.Y, FMath::CeilToInt(LocalBounds.Max.Y));
	Query.StrideX = Size.X;

	Query.IndexToBrush = IndexToBrush;
	Query.ScaleZ = ScaleZ;
	Query.OffsetZ = OffsetZ;

	Query.BlendMode = BlendMode;
	Query.Smoothness = Smoothness;

	Query.MinHeight = MinHeight;
	Query.MaxHeight = MaxHeight;

	return Query;
}

void FVoxelLandscapeHeightBrushRuntime::Destroy()
{
	VOXEL_FUNCTION_COUNTER();

	if (BrushIndex == -1)
	{
		return;
	}

	FVoxelLandscapeBrushManager::Get(World)->UnregisterHeightBrush(BrushIndex);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelLandscapeHeightBrushRuntime::FVoxelLandscapeHeightBrushRuntime(const AVoxelLandscapeHeightBrush& Brush)
	: World(Brush.GetWorld())
	, LocalToWorld(FVoxelUtilities::MakeTransform2(Brush.ActorToWorld()))
	, ScaleZ(Brush.ActorToWorld().GetScale3D().Z)
	, OffsetZ(Brush.ActorToWorld().GetTranslation().Z)
	, BlendMode(Brush.BlendMode)
	, Smoothness(Brush.Smoothness)
	, Priority(Brush.Priority)
	, HeightProvider(Brush.Height.MakeSharedCopy())
	, MinHeight(Brush.MinHeight)
	, MaxHeight(Brush.MaxHeight)
{
}

void FVoxelLandscapeHeightBrushRuntime::RegisterBrush()
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelBox2D NewBounds = HeightProvider->GetBounds();
	if (!ensure(NewBounds.IsValidAndNotEmpty()))
	{
		return;
	}

	// TODO Scale?
	PrivateLocalBounds = NewBounds;

	ensure(BrushIndex == -1);
	BrushIndex = FVoxelLandscapeBrushManager::Get(World)->RegisterHeightBrush(SharedThis(this));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void AVoxelLandscapeHeightBrush::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	QueueRecreate();
}
#endif

void AVoxelLandscapeHeightBrush::NotifyTransformChanged()
{
	QueueRecreate();
}

bool AVoxelLandscapeHeightBrush::ShouldDestroyWhenHidden() const
{
	return true;
}

FVoxelBox AVoxelLandscapeHeightBrush::GetLocalBounds() const
{
	VOXEL_FUNCTION_COUNTER();

	if (!Height)
	{
		return {};
	}

	const TSharedRef<FVoxelLandscapeHeightProvider> HeightCopy = Height.MakeSharedCopy();
	if (!HeightCopy->TryInitialize())
	{
		return {};
	}

	return HeightCopy->GetBounds().ToBox3D(0, 0);
}

TSharedPtr<IVoxelActorRuntime> AVoxelLandscapeHeightBrush::CreateNewRuntime()
{
	return FVoxelLandscapeHeightBrushRuntime::Create(*this);
}