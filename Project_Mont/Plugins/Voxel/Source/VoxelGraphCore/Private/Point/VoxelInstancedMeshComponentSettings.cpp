// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelInstancedMeshComponentSettings.h"
#include "Point/VoxelHierarchicalMeshComponent.h"

void FVoxelInstancedMeshComponentSettings::ApplyToComponent(UInstancedStaticMeshComponent& Component) const
{
	VOXEL_FUNCTION_COUNTER();

#define COPY(VariableName) \
	Component.VariableName = VariableName;

	Component.OverrideMaterials = MaterialOverrides;
	COPY(bCastDistanceFieldIndirectShadow);
	COPY(DistanceFieldIndirectShadowMinVisibility);
	COPY(bOverrideDistanceFieldSelfShadowBias);
	COPY(DistanceFieldSelfShadowBias);
	COPY(bSortTriangles);
	COPY(bReverseCulling);
	COPY(bOverrideLightMapRes);
	COPY(OverriddenLightMapRes);
	COPY(bOverrideWireframeColor);
	COPY(WireframeColorOverride);
	UE_503_ONLY(COPY(bForceNaniteForMasked));
	COPY(bDisallowNanite);
	COPY(bEvaluateWorldPositionOffset);
	UE_503_ONLY(COPY(bWorldPositionOffsetWritesVelocity));
	COPY(WorldPositionOffsetDisableDistance);
	COPY(bDisallowMeshPaintPerInstance);
	COPY(OverlayMaterial);
	COPY(OverlayMaterialMaxDrawDistance);
	COPY(BoundsScale);

#undef COPY

	Super::ApplyToComponent(Component);
}