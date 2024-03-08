// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Materials/MaterialRelevance.h"
#include "VoxelLandscapeVolumeComponent.generated.h"

class FVoxelLandscapeVolumeMesh;

UCLASS()
class VOXELLANDSCAPE_API UVoxelLandscapeVolumeComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	FVector BoundsExtension = FVector::ZeroVector;

	UVoxelLandscapeVolumeComponent();

	const TSharedPtr<FVoxelLandscapeVolumeMesh>& GetMesh() const
	{
		return Mesh;
	}
	void SetMesh(const TSharedPtr<FVoxelLandscapeVolumeMesh>& NewMesh);

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
	TSharedPtr<FVoxelLandscapeVolumeMesh> Mesh;

	friend class FVoxelLandscapeVolumeSceneProxy;
};