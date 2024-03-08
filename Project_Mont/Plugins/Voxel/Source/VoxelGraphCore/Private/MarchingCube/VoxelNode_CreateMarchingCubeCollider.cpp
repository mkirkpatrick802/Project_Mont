// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelNode_CreateMarchingCubeCollider.h"
#include "VoxelPositionQueryParameter.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_CreateMarchingCubeCollider, Collider)
{
	const TValue<FVoxelMarchingCubeSurface> Surface = Get(SurfacePin, Query);

	return VOXEL_ON_COMPLETE(Surface)
	{
		if (Surface->Vertices.Num() == 0)
		{
			return {};
		}

		check(Surface->Indices.Num() % 3 == 0);
		const int32 NumTriangles = Surface->Indices.Num() / 3;

		TValue<FVoxelPhysicalMaterialBuffer> Materials;
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelPositionQueryParameter>().Initialize([Surface, NumTriangles]
			{
				FVoxelFloatBufferStorage X; X.Allocate(NumTriangles);
				FVoxelFloatBufferStorage Y; Y.Allocate(NumTriangles);
				FVoxelFloatBufferStorage Z; Z.Allocate(NumTriangles);

				for (int32 Index = 0; Index < NumTriangles; Index++)
				{
					const FVector3f A = Surface->Vertices[Surface->Indices[3 * Index + 0]] * Surface->ScaledVoxelSize;
					const FVector3f B = Surface->Vertices[Surface->Indices[3 * Index + 1]] * Surface->ScaledVoxelSize;
					const FVector3f C = Surface->Vertices[Surface->Indices[3 * Index + 2]] * Surface->ScaledVoxelSize;

					const FVector3f Center = FVector3f(Surface->ChunkBounds.Min) + (A + B + C) / 3.f;

					X[Index] = Center.X;
					Y[Index] = Center.Y;
					Z[Index] = Center.Z;
				}

				return FVoxelVectorBuffer::Make(X, Y, Z);
			});

			Materials = Get(PhysicalMaterialPin, Query.MakeNewQuery(Parameters));
		}

		return VOXEL_ON_COMPLETE(Surface, Materials)
		{
			TVoxelArray<FVector3f> Positions;
			FVoxelUtilities::SetNumFast(Positions, Surface->Vertices.Num());

			for (int32 Index = 0; Index < Positions.Num(); Index++)
			{
				Positions[Index] = Surface->Vertices[Index] * Surface->ScaledVoxelSize;
			}

			if (Materials.IsConstant())
			{
				const TSharedPtr<FVoxelMarchingCubeCollider> Collider = FVoxelMarchingCubeCollider::Create(
					Surface->ChunkBounds.Min,
					Surface->Indices,
					Positions,
					{},
					{ Materials.GetConstant().Material });

				if (!Collider)
				{
					return {};
				}

				return FVoxelMarchingCubeColliderWrapper{ Collider };
			}

			TVoxelArray<uint16> MaterialIndices;
			TVoxelMap<uint64, uint16> MaterialToIndex;
			TVoxelArray<TWeakObjectPtr<UPhysicalMaterial>> PhysicalMaterials;

			for (const FVoxelPhysicalMaterial& Material : Materials)
			{
				uint16* IndexPtr = MaterialToIndex.Find(ReinterpretCastRef<uint64>(Material));
				if (!IndexPtr)
				{
					IndexPtr = &MaterialToIndex.Add_CheckNew(ReinterpretCastRef<uint64>(Material));
					*IndexPtr = PhysicalMaterials.Add(Material.Material);
				}
				MaterialIndices.Add(*IndexPtr);
			}

			const TSharedPtr<FVoxelMarchingCubeCollider> Collider = FVoxelMarchingCubeCollider::Create(
				Surface->ChunkBounds.Min,
				Surface->Indices,
				Positions,
				MaterialIndices,
				MoveTemp(PhysicalMaterials));

			if (!Collider)
			{
				return {};
			}

			return FVoxelMarchingCubeColliderWrapper{ Collider };
		};
	};
}