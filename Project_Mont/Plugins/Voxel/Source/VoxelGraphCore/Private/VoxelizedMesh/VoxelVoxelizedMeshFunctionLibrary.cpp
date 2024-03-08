// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelizedMesh/VoxelVoxelizedMeshFunctionLibrary.h"
#include "VoxelGraphMigration.h"
#include "VoxelDependencyTracker.h"
#include "VoxelPositionQueryParameter.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshData.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAssetData.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshAssetUserData.h"
#include "VoxelVoxelizedMeshFunctionLibraryImpl.ispc.generated.h"

VOXEL_RUN_ON_STARTUP_GAME(VoxelVoxelizedSurfaceMigration)
{
	REGISTER_VOXEL_FUNCTION_MIGRATION("Create Voxelized Mesh Surface", UVoxelVoxelizedMeshFunctionLibrary, CreateVoxelizedMeshDistanceField);
	REGISTER_VOXEL_FUNCTION_PIN_MIGRATION(UVoxelVoxelizedMeshFunctionLibrary, CreateVoxelizedMeshDistanceField, HermiteInterpolation, bHermiteInterpolation)
	REGISTER_VOXEL_FUNCTION_PIN_MIGRATION(UVoxelVoxelizedMeshFunctionLibrary, CreateVoxelizedMeshDistanceField, Surface, ReturnValue)
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelVoxelizedMeshPinType::Convert(
	const bool bSetObject,
	TWeakObjectPtr<UVoxelVoxelizedMeshAsset>& Object,
	FVoxelVoxelizedMesh& Struct) const
{
	if (bSetObject)
	{
		Object = Struct.Asset;
	}
	else
	{
		Struct.Asset = Object;
		Struct.Data = FVoxelVoxelizedMeshAssetData::Create(*Object);
	}
}

void FVoxelVoxelizedMeshStaticMeshPinType::Convert(
	const bool bSetObject,
	TWeakObjectPtr<UStaticMesh>& Object,
	FVoxelVoxelizedMeshStaticMesh& Struct) const
{
	if (bSetObject)
	{
		Object = Struct.StaticMesh;
	}
	else
	{
		Struct.StaticMesh = Object;

		if (UVoxelVoxelizedMeshAsset* Asset = UVoxelVoxelizedMeshAssetUserData::GetAsset(*Object))
		{
			Struct.VoxelizedMesh.Asset = Asset;
			Struct.VoxelizedMesh.Data = FVoxelVoxelizedMeshAssetData::Create(*Asset);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelVoxelizedMesh UVoxelVoxelizedMeshFunctionLibrary::MakeVoxelizedMeshFromStaticMesh(const FVoxelVoxelizedMeshStaticMesh& StaticMesh) const
{
	if (StaticMesh.StaticMesh.IsExplicitlyNull())
	{
		VOXEL_MESSAGE(Error, "{0}: mesh is null", this);
		return {};
	}

	return StaticMesh.VoxelizedMesh;
}

FVoxelFloatBuffer UVoxelVoxelizedMeshFunctionLibrary::CreateVoxelizedMeshDistanceField(
	FVoxelBox& Bounds,
	const FVoxelVoxelizedMesh& Mesh,
	const bool bHermiteInterpolation) const
{
	VOXEL_FUNCTION_COUNTER();

	if (Mesh.Asset.IsExplicitlyNull())
	{
		VOXEL_MESSAGE(Error, "{0}: mesh is null", this);
		return {};
	}

	GetQuery().GetDependencyTracker().AddDependency(Mesh.Data->Dependency);

	const TSharedPtr<const FVoxelVoxelizedMeshData> MeshData = Mesh.Data->GetMeshData();
	if (!MeshData)
	{
		VOXEL_MESSAGE(Error, "{0}: {1}: mesh failed to voxelize", this, Mesh.Asset);
		return {};
	}

	Bounds = MeshData->MeshBounds.Extend(1).Scale(MeshData->VoxelSize);

	FindOptionalVoxelQueryParameter_Function(FVoxelPositionQueryParameter, PositionQueryParameter);
	if (!PositionQueryParameter)
	{
		return {};
	}

	const FVoxelVectorBuffer Positions = PositionQueryParameter->GetPositions();
	const FIntVector Size = MeshData->Size;
	check(MeshData->DistanceField.Num() == Size.X * Size.Y * Size.Z);
	check(MeshData->Normals.Num() == Size.X * Size.Y * Size.Z);

	FVoxelFloatBufferStorage Distance;
	Distance.Allocate(Positions.Num());

	ForeachVoxelBufferChunk_Parallel(Positions.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		ispc::VoxelVoxelizedMeshFunctionLibrary_SampleVoxelizedMesh(
			Positions.X.GetData(Iterator),
			Positions.Y.GetData(Iterator),
			Positions.Z.GetData(Iterator),
			Positions.X.IsConstant(),
			Positions.Y.IsConstant(),
			Positions.Z.IsConstant(),
			Iterator.Num(),
			GetISPCValue(Size),
			MeshData->DistanceField.GetData(),
			MeshData->Normals.GetData(),
			GetISPCValue(MeshData->Origin),
			MeshData->VoxelSize,
			bHermiteInterpolation,
			Distance.GetData(Iterator));
	});

	return FVoxelFloatBuffer::Make(Distance);
}