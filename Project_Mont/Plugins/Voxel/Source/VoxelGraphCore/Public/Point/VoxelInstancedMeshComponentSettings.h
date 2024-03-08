// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPrimitiveComponentSettings.h"
#include "VoxelInstancedMeshComponentSettings.generated.h"

USTRUCT(BlueprintType)
struct VOXELGRAPHCORE_API FVoxelInstancedMeshComponentSettings : public FVoxelPrimitiveComponentSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta = (ShowOverride = IfOverriden))
	TArray<TObjectPtr<UMaterialInterface>> MaterialOverrides;

	// Whether to use the mesh distance field representation (when present) for shadowing indirect lighting (from lightmaps or skylight) on Movable components.
	// This works like capsule shadows on skeletal meshes, except using the mesh distance field so no physics asset is required.
	// The StaticMesh must have 'Generate Mesh Distance Field' enabled, or the project must have 'Generate Mesh Distance Fields' enabled for this feature to work.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (DisplayName = "Distance Field Indirect Shadow"))
	bool bCastDistanceFieldIndirectShadow = false;

	// Controls how dark the dynamic indirect shadow can be.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (UIMin = "0", UIMax = "1", DisplayName = "Distance Field Indirect Shadow Min Visibility", EditCondition = "bCastDistanceFieldIndirectShadow"))
	float DistanceFieldIndirectShadowMinVisibility = 0.1f;

	// Whether to override the DistanceFieldSelfShadowBias setting of the static mesh asset with the DistanceFieldSelfShadowBias of this component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bOverrideDistanceFieldSelfShadowBias = false;

	// Useful for reducing self shadowing from distance field methods when using world position offset to animate the mesh's vertices.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (EditCondition = "bOverrideDistanceFieldSelfShadowBias"))
	float DistanceFieldSelfShadowBias = 0.f;

	// Enable dynamic sort mesh's triangles to remove ordering issue when rendered with a translucent material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (UIMin = "0", UIMax = "1", DisplayName = "Sort Triangles (Experimental)"))
	bool bSortTriangles = false;

	// Controls whether the static mesh component's backface culling should be reversed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bReverseCulling = false;

	// Whether to override the lightmap resolution defined in the static mesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting")
	bool bOverrideLightMapRes = false;

	// Light map resolution to use on this component, used if bOverrideLightMapRes is true and there is a valid StaticMesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lighting", meta = (ClampMax = 4096, EditCondition="bOverrideLightMapRes"))
	int32 OverriddenLightMapRes = 64;
	// If true, WireframeColorOverride will be used. If false, color is determined based on mobility and physics simulation settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bOverrideWireframeColor = false;

	// Wireframe color to use if bOverrideWireframeColor is true
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (EditCondition = "bOverrideWireframeColor"))
	FColor WireframeColorOverride = FColor::White;

	// Forces this component to always use Nanite for masked materials, even if FNaniteSettings::bAllowMaskedMaterials=false
	// NOTE: Does nothing in 5.2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bForceNaniteForMasked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bDisallowNanite = false;

	// Whether to evaluate World Position Offset.
	// This is only used when running with r.OptimizedWPO=1
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bEvaluateWorldPositionOffset = true;

	// Whether world position offset turns on velocity writes.
	// If the WPO isn't static then setting false may give incorrect motion vectors.
	// But if we know that the WPO is static then setting false may save performance.
	// NOTE: Does nothing in 5.2
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bWorldPositionOffsetWritesVelocity = true;

	// Distance at which to disable World Position Offset for an entire instance (0 = Never disable WPO).
	// NOTE: Currently works with Nanite only.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	int32 WorldPositionOffsetDisableDistance = 0;

	// If true, mesh painting is disallowed on this instance. Set if vertex colors are overridden in a construction script.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	bool bDisallowMeshPaintPerInstance = true;

	// Translucent material to blend on top of this mesh. Mesh will be rendered twice - once with a base material and once with overlay material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	// The max draw distance for overlay material. A distance of 0 indicates that overlay will be culled using primitive max distance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering")
	float OverlayMaterialMaxDrawDistance = 0.f;

	// Scales the bounds of the object.
	// This is useful when using World Position Offset to animate the vertices of the object outside of its bounds.
	// Warning: Increasing the bounds of an object will reduce performance and shadow quality!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rendering", meta = (UIMin = "1", UIMax = "10.0"))
	float BoundsScale = 1.f;

	void ApplyToComponent(UInstancedStaticMeshComponent& Component) const;
};