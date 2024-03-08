// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelMarchingCubeNavmeshComponent.generated.h"

class FVoxelMarchingCubeNavmesh;

UCLASS()
class VOXELGRAPHCORE_API UVoxelMarchingCubeNavmeshComponent final : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UVoxelMarchingCubeNavmeshComponent();

	void SetNavigationMesh(const TSharedPtr<const FVoxelMarchingCubeNavmesh>& NewNavmesh);
	void ReturnToPool();

	//~ Begin UPrimitiveComponent Interface
	virtual bool ShouldCreatePhysicsState() const override { return false; }
	virtual bool IsNavigationRelevant() const override;
	virtual bool DoCustomNavigableGeometryExport(FNavigableGeometryExport& GeomExport) const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ End UPrimitiveComponent Interface

private:
	TSharedPtr<const FVoxelMarchingCubeNavmesh> Navmesh;
};