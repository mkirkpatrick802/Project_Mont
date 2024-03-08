// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelSurface.h"
#include "VoxelNode_GetSurfaceAttributes.generated.h"

USTRUCT(Category = "Surface", meta = (ShowInShortList))
struct VOXELGRAPHCORE_API FVoxelNode_GetSurfaceAttributes : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

public:
	VOXEL_INPUT_PIN(FVoxelSurface, Surface, nullptr);

public:
	FVoxelNode_GetSurfaceAttributes();

	//~ Begin FVoxelNode Interface
	virtual void PostSerialize() override;
	virtual FVoxelComputeValue CompileCompute(FName PinName) const override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End FVoxelNode Interface

public:
	UPROPERTY(EditAnywhere, Category = "Config", meta = (TitleProperty = "Name"))
	TArray<FVoxelSurfaceAttributeInfo> Attributes =
	{
		FVoxelSurfaceAttributeInfo()
	};

	void FixupPins();
};