// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPointSet.h"
#include "VoxelNodeInterface.h"
#include "VoxelPointSpawner.generated.h"

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelPointSpawner
	: public FVoxelVirtualStruct
	, public IVoxelNodeInterface
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

public:
	FVoxelGraphNodeRef NodeRef;

	//~ Begin IVoxelNodeInterface Interface
	virtual const FVoxelGraphNodeRef& GetNodeRef() const override
	{
		return NodeRef;
	}
	//~ End IVoxelNodeInterface Interface

public:
	virtual TSharedPtr<const FVoxelPointSet> GeneratePoints(const FVoxelPointSet& Points) const VOXEL_PURE_VIRTUAL({});
};