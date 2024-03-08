// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeVolumeProvider.generated.h"

enum class EVoxelLandscapeVolumeBlendMode : uint8;

struct FVoxelLandscapeVolumeQuery
{
	TVoxelArrayView<float> Distances;
	FVoxelIntBox Span;
	int32 StrideX = 0;
	int32 StrideXY = 0;

	FMatrix IndexToBrush;
	float DistanceScale = 1.f;

	EVoxelLandscapeVolumeBlendMode BlendMode = {};
	float Smoothness = 0.f;
};

USTRUCT()
struct FVoxelLandscapeVolumeProvider
	: public FVoxelVirtualStruct
	, public TSharedFromThis<FVoxelLandscapeVolumeProvider>
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual bool TryInitialize() VOXEL_PURE_VIRTUAL({});
	virtual FVoxelBox GetBounds() const VOXEL_PURE_VIRTUAL({});
	virtual FVoxelFuture Apply(const FVoxelLandscapeVolumeQuery& Query) const VOXEL_PURE_VIRTUAL({});
};