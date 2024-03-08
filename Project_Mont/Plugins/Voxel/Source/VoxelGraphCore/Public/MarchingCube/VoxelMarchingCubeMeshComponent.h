// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Materials/MaterialRelevance.h"
#include "VoxelMarchingCubeMeshComponent.generated.h"

class FVoxelMarchingCubeMesh;

UCLASS()
class VOXELGRAPHCORE_API UVoxelMarchingCubeMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	bool bOnlyDrawIfSelected = false;
	FVector BoundsExtension = FVector::ZeroVector;

	UVoxelMarchingCubeMeshComponent();

	const TSharedPtr<FVoxelMarchingCubeMesh>& GetMesh() const
	{
		return Mesh;
	}
	void SetMesh(const TSharedPtr<FVoxelMarchingCubeMesh>& NewMesh);

	void ReturnToPool();

public:
	//~ Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldCreatePhysicsState() const override { return false; }

	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const override;

	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ End UPrimitiveComponent Interface

	FMaterialRelevance GetMaterialRelevance(ERHIFeatureLevel::Type InFeatureLevel) const;

private:
	TSharedPtr<FVoxelMarchingCubeMesh> Mesh;

	friend class FVoxelMarchingCubeMeshSceneProxy;
};