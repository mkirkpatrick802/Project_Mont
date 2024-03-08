// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Volume/Mesh/VoxelLandscapeMeshVolumeProvider.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshData.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAsset.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAssetUserData.h"
#include "VoxelLandscapeMeshVolumeProviderImpl.ispc.generated.h"

bool FVoxelLandscapeMeshVolumeProvider::TryInitialize()
{
	if (!Mesh)
	{
		return false;
	}

	UVoxelVoxelizedMeshAsset* VoxelizedMesh = UVoxelVoxelizedMeshAssetUserData::GetAsset(*Mesh);
	if (!ensureVoxelSlow(VoxelizedMesh))
	{
		return false;
	}

	MeshData = VoxelizedMesh->GetData();
	if (!ensureVoxelSlow(MeshData))
	{
		return false;
	}

	return true;
}

FVoxelBox FVoxelLandscapeMeshVolumeProvider::GetBounds() const
{
	return MeshData->MeshBounds.Extend(1).Scale(MeshData->VoxelSize);
}

FVoxelFuture FVoxelLandscapeMeshVolumeProvider::Apply(const FVoxelLandscapeVolumeQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER_NUM(Query.Span.Count_SmallBox(), 0);

	const FTransform IndexToMesh = FVoxelUtilities::MakeTransformSafe(
		Query.IndexToBrush *
		FScaleMatrix(1. / MeshData->VoxelSize) *
		FTranslationMatrix(-FVector(MeshData->Origin)));

	ispc::VoxelLandscapeMeshVolumeProvider_Apply(
		Query.Distances.GetData(),
		Query.Span.Min.X,
		Query.Span.Min.Y,
		Query.Span.Min.Z,
		Query.Span.Max.X,
		Query.Span.Max.Y,
		Query.Span.Max.Z,
		Query.StrideX,
		Query.StrideXY,
		GetISPCValue(IndexToMesh.GetTranslation()),
		GetISPCValue(FVector4d(
			IndexToMesh.GetRotation().X,
			IndexToMesh.GetRotation().Y,
			IndexToMesh.GetRotation().Z,
			IndexToMesh.GetRotation().W)),
		GetISPCValue(IndexToMesh.GetScale3D()),
		ispc::EVoxelLandscapeVolumeBlendMode(Query.BlendMode),
		Query.Smoothness,
		GetISPCValue(MeshData->Size),
		MeshData->VoxelSize * Query.DistanceScale,
		MeshData->DistanceField.GetData());

	return FVoxelFuture::Done();
}