// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "PrimitiveSceneProxy.h"

class FVoxelLandscapeVolumeMesh;
class UVoxelLandscapeVolumeComponent;

class FVoxelLandscapeVolumeSceneProxy : public FPrimitiveSceneProxy
{
public:
	explicit FVoxelLandscapeVolumeSceneProxy(const UVoxelLandscapeVolumeComponent& Component);

	//~ Begin FPrimitiveSceneProxy Interface
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual bool CanBeOccluded() const override;
	virtual uint32 GetMemoryFootprint() const override;
	virtual SIZE_T GetTypeHash() const override;

	virtual void GetDistanceFieldAtlasData(const FDistanceFieldVolumeData*& OutDistanceFieldData, float& SelfShadowBias) const override;
	virtual void GetDistanceFieldInstanceData(TArray<FRenderTransform>& ObjectLocalToWorldTransforms) const override;

	virtual const FCardRepresentationData* GetMeshCardRepresentation() const override;
	virtual bool HasDistanceFieldRepresentation() const override;

	virtual bool HasDynamicIndirectShadowCasterRepresentation() const override;
	//~ End FPrimitiveSceneProxy Interface

private:
	const FMaterialRelevance MaterialRelevance;
	const TSharedRef<FVoxelLandscapeVolumeMesh> Mesh;

	TQueue<TSharedPtr<FVoxelMaterialRef>, EQueueMode::Mpsc> MaterialsToKeepAlive;

	static bool ShouldUseStaticPath(const FSceneViewFamily& ViewFamily);

	bool DrawMesh(
		FMeshBatch& MeshBatch,
		const FMaterialRenderProxy* MaterialRenderProxy,
		bool bWireframe) const;
};