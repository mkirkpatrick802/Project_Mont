// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "RawIndexBuffer.h"
#include "PrimitiveSceneProxy.h"
#include "Rendering/ColorVertexBuffer.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Rendering/StaticMeshVertexBuffer.h"

class FLocalVertexFactory;
class FVoxelMarchingCubeCollider;
class UVoxelMarchingCubeCollisionComponent;

class FVoxelMarchingCubeColliderRenderData
{
public:
	explicit FVoxelMarchingCubeColliderRenderData(const FVoxelMarchingCubeCollider& Collider);
	~FVoxelMarchingCubeColliderRenderData();

	void Draw_RenderThread(FMeshBatch& MeshBatch) const;
private:
	FRawStaticIndexBuffer IndexBuffer{ false };
	FPositionVertexBuffer PositionVertexBuffer;
	FStaticMeshVertexBuffer StaticMeshVertexBuffer;
	FColorVertexBuffer ColorVertexBuffer;
	TUniquePtr<FLocalVertexFactory> VertexFactory;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelMarchingCubeCollisionSceneProxy : public FPrimitiveSceneProxy
{
public:
	const TSharedRef<const FVoxelMarchingCubeCollider> Collider;
	mutable TSharedPtr<FVoxelMarchingCubeColliderRenderData> RenderData;

	explicit FVoxelMarchingCubeCollisionSceneProxy(const UVoxelMarchingCubeCollisionComponent& Component);

	//~ Begin FPrimitiveSceneProxy Interface
	virtual void DestroyRenderThreadResources() override;
	virtual void GetDynamicMeshElements(
		const TArray<const FSceneView*>& Views,
		const FSceneViewFamily& ViewFamily,
		uint32 VisibilityMap,
		FMeshElementCollector& Collector) const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual uint32 GetMemoryFootprint() const override;
	virtual SIZE_T GetTypeHash() const override;
	//~ End FPrimitiveSceneProxy Interface
};