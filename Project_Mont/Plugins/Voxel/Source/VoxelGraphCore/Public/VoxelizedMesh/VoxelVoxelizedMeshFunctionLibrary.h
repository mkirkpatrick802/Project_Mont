// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "Engine/StaticMesh.h"
#include "VoxelObjectPinType.h"
#include "VoxelFunctionLibrary.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAsset.h"
#include "VoxelVoxelizedMeshFunctionLibrary.generated.h"

class FVoxelVoxelizedMeshAssetData;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelVoxelizedMesh
{
	GENERATED_BODY()

	TWeakObjectPtr<UVoxelVoxelizedMeshAsset> Asset;
	TSharedPtr<const FVoxelVoxelizedMeshAssetData> Data;
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelVoxelizedMeshStaticMesh
{
	GENERATED_BODY()

	TWeakObjectPtr<UStaticMesh> StaticMesh;
	FVoxelVoxelizedMesh VoxelizedMesh;
};

DECLARE_VOXEL_OBJECT_PIN_TYPE(FVoxelVoxelizedMesh);

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelVoxelizedMeshPinType : public FVoxelObjectPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_OBJECT_PIN_TYPE(FVoxelVoxelizedMesh, UVoxelVoxelizedMeshAsset);
};

DECLARE_VOXEL_OBJECT_PIN_TYPE(FVoxelVoxelizedMeshStaticMesh);

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelVoxelizedMeshStaticMeshPinType : public FVoxelObjectPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_OBJECT_PIN_TYPE(FVoxelVoxelizedMeshStaticMesh, UStaticMesh);
};

UCLASS()
class VOXELGRAPHCORE_API UVoxelVoxelizedMeshFunctionLibrary : public UVoxelFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(Category = "Voxelized Mesh")
	FVoxelVoxelizedMesh MakeVoxelizedMeshFromStaticMesh(const FVoxelVoxelizedMeshStaticMesh& StaticMesh) const;

	// Creates a distance field from a voxelized static mesh
	// @param	bHermiteInterpolation	Enabling hermite interpolation can lead to higher quality results (less artifacts when interpolating),
	//									but is up to 4x more expensive
	UFUNCTION(Category = "Voxelized Mesh", meta = (Keywords = "make"))
	UPARAM(DisplayName = "Distance") FVoxelFloatBuffer CreateVoxelizedMeshDistanceField(
		FVoxelBox& Bounds,
		const FVoxelVoxelizedMesh& Mesh,
		bool bHermiteInterpolation = false) const;
};