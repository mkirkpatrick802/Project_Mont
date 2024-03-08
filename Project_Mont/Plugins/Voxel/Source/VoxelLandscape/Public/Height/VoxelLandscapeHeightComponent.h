// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Materials/MaterialRelevance.h"
#include "VoxelLandscapeHeightComponent.generated.h"

class FVoxelLandscapeHeightMesh;

UCLASS()
class VOXELLANDSCAPE_API UVoxelLandscapeHeightComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	// TODO
	FVector BoundsExtension = FVector(ForceInit);

	UVoxelLandscapeHeightComponent();

	const TSharedPtr<FVoxelLandscapeHeightMesh>& GetMesh() const
	{
		return Mesh;
	}
	void SetMesh(const TSharedPtr<FVoxelLandscapeHeightMesh>& NewMesh);

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
	TSharedPtr<FVoxelLandscapeHeightMesh> Mesh;

	friend class FVoxelLandscapeHeightSceneProxy;
};