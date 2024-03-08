// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMarchingCubeCollisionComponent.generated.h"

class FVoxelMarchingCubeCollider;

UCLASS()
class VOXELGRAPHCORE_API UVoxelMarchingCubeCollisionComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelMarchingCubeCollisionComponent() = default;

	void SetBodyInstance(const FBodyInstance& NewBodyInstance);
	void SetCollider(const TSharedPtr<const FVoxelMarchingCubeCollider>& NewCollider);
	void ReturnToPool();

	//~ Begin UPrimitiveComponent Interface
	virtual UBodySetup* GetBodySetup() override { return BodySetup; }
	virtual bool ShouldCreatePhysicsState() const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	//~ End UPrimitiveComponent Interface

private:
	UPROPERTY(Transient)
	TObjectPtr<UBodySetup> BodySetup;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> Materials;

	TSharedPtr<const FVoxelMarchingCubeCollider> Collider;

	friend class FVoxelMarchingCubeCollisionSceneProxy;
};