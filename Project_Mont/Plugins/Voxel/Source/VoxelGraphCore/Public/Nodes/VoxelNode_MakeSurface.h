// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurface.h"
#include "VoxelNode_MakeSurface.generated.h"

USTRUCT(meta = (Abstract))
struct VOXELGRAPHCORE_API FVoxelNode_MakeSurface : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelBox, Bounds, nullptr);
	VOXEL_OUTPUT_PIN(FVoxelSurface, Surface);

	//~ Begin FVoxelNode Interface
	virtual void PostSerialize() override;
	virtual FVoxelComputeValue CompileCompute(FName PinName) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End FVoxelNode Interface

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (TitleProperty = "Name"))
	TArray<FVoxelSurfaceAttributeInfo> Attributes;

	void FixupPins();
};

USTRUCT(DisplayName = "Make 2D Surface", Category = "Surface", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_Make2DSurface : public FVoxelNode_MakeSurface
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	FVoxelNode_Make2DSurface();
};

USTRUCT(DisplayName = "Make 3D Surface", Category = "Surface", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_Make3DSurface : public FVoxelNode_MakeSurface
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	FVoxelNode_Make3DSurface();
};