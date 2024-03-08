// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "VoxelLandscapeNaniteComponent.generated.h"

class FVoxelLandscapeNaniteMesh;

UCLASS()
class VOXELLANDSCAPE_API UVoxelLandscapeNaniteComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	const TSharedPtr<FVoxelLandscapeNaniteMesh>& GetMesh() const
	{
		return Mesh;
	}
	void SetMesh(const TSharedPtr<FVoxelLandscapeNaniteMesh>& NewMesh);
	
	//~ Begin UPrimitiveComponent Interface
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	//~ End UPrimitiveComponent Interface

private:
	TSharedPtr<FVoxelLandscapeNaniteMesh> Mesh;
};