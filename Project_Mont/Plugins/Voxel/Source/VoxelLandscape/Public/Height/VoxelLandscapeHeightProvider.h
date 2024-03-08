// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelLandscapeHeightProvider.generated.h"

enum class EVoxelLandscapeHeightBlendMode : uint8;

struct FVoxelLandscapeHeightQuery
{
	TVoxelArrayView<float> Heights;
	FIntRect Span;
	int32 StrideX = 0;

	FTransform2d IndexToBrush = FTransform2d(ForceInit);
	float ScaleZ = 1;
	float OffsetZ = 0;

	EVoxelLandscapeHeightBlendMode BlendMode = {};
	float Smoothness = 0.f;

	float MinHeight = 0.f;
	float MaxHeight = 0.f;
};

USTRUCT()
struct FVoxelLandscapeHeightProvider
	: public FVoxelVirtualStruct
	, public TSharedFromThis<FVoxelLandscapeHeightProvider>
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	virtual bool TryInitialize() VOXEL_PURE_VIRTUAL({});

	virtual FVoxelBox2D GetBounds() const VOXEL_PURE_VIRTUAL({});
	virtual FVoxelFuture Apply(const FVoxelLandscapeHeightQuery& Query) const VOXEL_PURE_VIRTUAL({});
};