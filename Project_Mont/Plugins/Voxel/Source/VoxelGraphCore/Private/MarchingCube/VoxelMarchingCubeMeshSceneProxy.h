// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "PrimitiveSceneProxy.h"

class FVoxelMarchingCubeMesh;
class UVoxelMarchingCubeMeshComponent;

class FVoxelMarchingCubeMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	explicit FVoxelMarchingCubeMeshSceneProxy(const UVoxelMarchingCubeMeshComponent& Component);

	//~ Begin FPrimitiveSceneProxy Interface
	virtual void CreateRenderThreadResources(UE_504_ONLY(FRHICommandListBase& RHICmdList)) override;

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
	const TSharedRef<FVoxelMarchingCubeMesh> Mesh;
	const bool bOnlyDrawIfSelected;
	const FObjectKey OwnerForSelectionCheck;

	TQueue<TSharedPtr<FVoxelMaterialRef>, EQueueMode::Mpsc> MaterialsToKeepAlive;

	bool ShouldUseStaticPath(const FSceneViewFamily& ViewFamily) const;

	bool DrawMesh(
		FMeshBatch& MeshBatch,
		const FMaterialRenderProxy* MaterialRenderProxy,
		bool bWireframe) const;
};