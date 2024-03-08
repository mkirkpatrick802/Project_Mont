// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscapeChunkTree.h"
#include "VoxelLandscapeState.h"
#include "VoxelLandscapeBrushManager.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELLANDSCAPE_API, int32, GVoxelLandscapeMaxRenderedChunk, 100000,
	"voxel.landscape.MaxRenderedChunks",
	"");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelLandscapeChunkTree::FVoxelLandscapeChunkTree(const FVoxelLandscapeState& State)
	: State(State)
	, CameraChunkPosition(State.VoxelToWorld.InverseTransformPosition(State.CameraPosition) / State.ChunkSize)
	, TargetChunkSizeOnScreenSquared(FMath::Square(State.TargetVoxelSizeOnScreen * 32. / State.ChunkSize))
{
}

void FVoxelLandscapeChunkTree::Build()
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelBox VoxelBounds = State.GetBrushes().GetBounds();
	if (!VoxelBounds.IsValidAndNotEmpty())
	{
		return;
	}

	const FVoxelBox ChunkBounds = VoxelBounds / State.ChunkSize;
	const double MaxSize = FMath::Max(ChunkBounds.Min.GetAbsMax(), ChunkBounds.Max.GetAbsMax());

	const int32 Depth = FMath::CeilLogTwo(FMath::CeilToInt(MaxSize));
	const int32 RootChunkSize = 1 << Depth;

	ensure(FVoxelBox(-RootChunkSize, RootChunkSize).Contains(ChunkBounds));
	ensure(!FVoxelBox(-RootChunkSize / 2, RootChunkSize / 2).Contains(ChunkBounds));

	{
		VOXEL_SCOPE_COUNTER("Traverse");

		const FIntVector Min = FIntVector(-RootChunkSize);
		const FIntVector Max = FIntVector(0);

		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Min.X, Min.Y, Min.Z) });
		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Max.X, Min.Y, Min.Z) });
		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Min.X, Max.Y, Min.Z) });
		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Max.X, Max.Y, Min.Z) });
		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Min.X, Min.Y, Max.Z) });
		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Max.X, Min.Y, Max.Z) });
		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Min.X, Max.Y, Max.Z) });
		Traverse(FVoxelLandscapeChunkKey{ Depth, FIntVector(Max.X, Max.Y, Max.Z) });
	}

	if (ChunkKeys.Num() >= GVoxelLandscapeMaxRenderedChunk)
	{
		VOXEL_MESSAGE(Warning, "More than {0} voxel chunks rendered - throttling", GVoxelLandscapeMaxRenderedChunk);
	}

	// TODO Build transition mask by virtually checking the neighbor resolution - ie, increase/decrease neighbor resolution until it is rendered
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelLandscapeChunkTree::Traverse(const FVoxelLandscapeChunkKey ChunkKey)
{
	if (ChunkKeys.Num() >= GVoxelLandscapeMaxRenderedChunk)
	{
		return;
	}

	const FVoxelBox Bounds =
		ChunkKey.GetChunkKeyBounds()
		.ToVoxelBox()
		.Scale(State.ChunkSize);

	if (!State.GetBrushes().Intersect(Bounds))
	{
		return;
	}

	if (ChunkKey.LOD == 0 ||
		!ShouldSubdivide(ChunkKey))
	{
		ChunkKeys.Add(FVoxelLandscapeChunkKeyWithTransition{ ChunkKey, 0 });
		return;
	}

	const int32 ChildLOD = ChunkKey.LOD - 1;

	const FIntVector Min = ChunkKey.ChunkKey;
	const FIntVector Max = ChunkKey.ChunkKey + (1 << ChildLOD);

	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Min.X, Min.Y, Min.Z) });
	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Max.X, Min.Y, Min.Z) });
	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Min.X, Max.Y, Min.Z) });
	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Max.X, Max.Y, Min.Z) });
	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Min.X, Min.Y, Max.Z) });
	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Max.X, Min.Y, Max.Z) });
	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Min.X, Max.Y, Max.Z) });
	Traverse(FVoxelLandscapeChunkKey{ ChildLOD, FIntVector(Max.X, Max.Y, Max.Z) });
}

FORCEINLINE bool FVoxelLandscapeChunkTree::ShouldSubdivide(const FVoxelLandscapeChunkKey ChunkKey) const
{
	checkVoxelSlow(ChunkKey.LOD > 0);

	const FVoxelIntBox Bounds = ChunkKey.GetChunkKeyBounds();

	const double DistanceToCameraSquared = Bounds.ComputeSquaredDistanceFromBoxToPoint(CameraChunkPosition);
	const double ChunkSizeOnScreenSquared = DistanceToCameraSquared == 0
		? FMath::Square(1e9)
		: FMath::Square((1 << ChunkKey.LOD)) / DistanceToCameraSquared;

	return ChunkSizeOnScreenSquared > TargetChunkSizeOnScreenSquared;
}