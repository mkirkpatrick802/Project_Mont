// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "MarchingCube/VoxelMarchingCubeCollider.h"
#include "VoxelAABBTree.h"
#include "Chaos/TriangleMeshImplicitObject.h"

DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelMarchingCubeColliderMemory);

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, bool, GVoxelCollisionFastCooking, true,
	"voxel.collision.FastCooking",
	"");

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace Chaos
{
	template<typename, typename>
	struct FTriangleMeshSweepVisitorCCD;

	template<>
	struct FTriangleMeshSweepVisitorCCD<void, void>
	{
		static int64 GetAllocatedSize(const FTriangleMeshImplicitObject& TriangleMesh)
		{
			int64 AllocatedSize = sizeof(FTriangleMeshImplicitObject);
			AllocatedSize += TriangleMesh.MParticles.GetAllocatedSize();

			if (TriangleMesh.MElements.RequiresLargeIndices())
			{
				AllocatedSize += TriangleMesh.MElements.GetLargeIndexBuffer().GetAllocatedSize();
			}
			else
			{
				AllocatedSize += TriangleMesh.MElements.GetSmallIndexBuffer().GetAllocatedSize();
			}

			AllocatedSize += TriangleMesh.MaterialIndices.GetAllocatedSize();

			if (TriangleMesh.ExternalFaceIndexMap)
			{
				AllocatedSize += TriangleMesh.ExternalFaceIndexMap->GetAllocatedSize();
			}
			if (TriangleMesh.ExternalVertexIndexMap)
			{
				AllocatedSize += TriangleMesh.ExternalVertexIndexMap->GetAllocatedSize();
			}

			AllocatedSize += TriangleMesh.FastBVH.Nodes.GetAllocatedSize();
			AllocatedSize += TriangleMesh.FastBVH.FaceBounds.GetAllocatedSize();

			return AllocatedSize;
		}
	};

	struct FCookTriangleDummy;

	template<>
	struct FTriangleMeshOverlapVisitorNoMTD<FCookTriangleDummy>
	{
		template<typename IndexType>
		static UE_504_SWITCH(TSharedPtr, TRefCountPtr)<FTriangleMeshImplicitObject> CookTriangleMesh(
			const TConstVoxelArrayView<int32> Indices,
			const TConstVoxelArrayView<FVector3f> Vertices,
			const TConstVoxelArrayView<uint16> FaceMaterials)
		{
			VOXEL_FUNCTION_COUNTER();
			VOXEL_ALLOW_MALLOC_SCOPE();

			TParticles<FRealSingle, 3> Particles;
			Particles.AddParticles(Vertices.Num());

			for (int32 Index = 0; Index < Vertices.Num(); Index++)
			{
				Particles.X(Index) = Vertices[Index];
			}

			const int32 NumTriangles = Indices.Num() / 3;

			TCompatibleVoxelArray<TVector<IndexType, 3>> Triangles;
			Triangles.Reserve(NumTriangles);

			for (int32 Index = 0; Index < NumTriangles; Index++)
			{
				const TVector<int32, 3> Triangle{
					Indices[3 * Index + 2],
					Indices[3 * Index + 1],
					Indices[3 * Index + 0]
				};

				if (!FConvexBuilder::IsValidTriangle(
					Particles.X(Triangle.X),
					Particles.X(Triangle.Y),
					Particles.X(Triangle.Z)))
				{
					continue;
				}

				Triangles.Add(Triangle);
			}

			if (Triangles.Num() == 0)
			{
				return nullptr;
			}

			if (!GVoxelCollisionFastCooking)
			{
				VOXEL_SCOPE_COUNTER("Slow cook");

#if VOXEL_ENGINE_VERSION < 504
				return MakeVoxelShared<FTriangleMeshImplicitObject>(
#else
				return new FTriangleMeshImplicitObject(
#endif
					MoveTemp(Particles),
					MoveTemp(Triangles),
					TArray<uint16>(FaceMaterials),
					nullptr,
					nullptr,
					true);
			}

			VOXEL_SCOPE_COUNTER("Fast cook");

			TVoxelArray<FVoxelAABBTree::FElement> Elements;
			FVoxelUtilities::SetNumFast(Elements, Triangles.Num());
			for (int32 Index = 0; Index < Triangles.Num(); Index++)
			{
				const TVector<IndexType, 3>& Triangle = Triangles[Index];
				const FVector3f VertexA = Vertices[Triangle.X];
				const FVector3f VertexB = Vertices[Triangle.Y];
				const FVector3f VertexC = Vertices[Triangle.Z];

				FVoxelAABBTree::FElement& Element = Elements[Index];
				Element.Payload = Index;
				Element.Bounds = FVoxelBox(
					FVoxelUtilities::ComponentMin3(VertexA, VertexB, VertexC),
					FVoxelUtilities::ComponentMax3(VertexA, VertexB, VertexC));
			}

			using FLeaf = TAABBTreeLeafArray<int32, false, float>;
			using FBVHType = TAABBTree<int32, FLeaf, false, float>;
			checkStatic(std::is_same_v<FBVHType, FTriangleMeshImplicitObject::BVHType>);

			// From FTriangleMeshImplicitObject::RebuildBVImp
			constexpr static int32 MaxChildrenInLeaf = 22;
			constexpr static int32 MaxTreeDepth = FBVHType::DefaultMaxTreeDepth;

			FVoxelAABBTree Tree(MaxChildrenInLeaf, MaxTreeDepth);
			Tree.Initialize(MoveTemp(Elements));

			const TConstVoxelArrayView<FVoxelAABBTree::FNode> SrcNodes = Tree.GetNodes();
			const TConstVoxelArrayView<FVoxelAABBTree::FLeaf> SrcLeaves = Tree.GetLeaves();

			FBVHType BVH;
			TCompatibleVoxelArray<TAABBTreeNode<float>>& DestNodes = ToCompatibleVoxelArray(ConstCast(BVH.GetNodes()));
			TCompatibleVoxelArray<FLeaf>& DestLeaves = ToCompatibleVoxelArray(ConstCast(BVH.GetLeaves()));

			FVoxelUtilities::SetNum(DestNodes, SrcNodes.Num());
			FVoxelUtilities::SetNum(DestLeaves, SrcLeaves.Num());

			for (int32 Index = 0; Index < SrcNodes.Num(); Index++)
			{
				const FVoxelAABBTree::FNode& SrcNode = SrcNodes[Index];
				TAABBTreeNode<float>& DestNode = DestNodes[Index];

				if (SrcNode.bLeaf)
				{
					DestNode.bLeaf = true;
					DestNode.ChildrenNodes[0] = SrcNode.LeafIndex;
				}
				else
				{
					DestNode.bLeaf = false;
					DestNode.ChildrenNodes[0] = SrcNode.ChildIndex0;
					DestNode.ChildrenNodes[1] = SrcNode.ChildIndex1;
					DestNode.ChildrenBounds[0] = FAABB3f(SrcNode.ChildBounds0.Min, SrcNode.ChildBounds0.Max);
					DestNode.ChildrenBounds[1] = FAABB3f(SrcNode.ChildBounds1.Min, SrcNode.ChildBounds1.Max);
				}
			}

			for (int32 Index = 0; Index < SrcLeaves.Num(); Index++)
			{
				const FVoxelAABBTree::FLeaf& SrcLeaf = SrcLeaves[Index];
				FLeaf& DestLeaf = DestLeaves[Index];

				FVoxelUtilities::SetNumFast(DestLeaf.Elems, SrcLeaf.Elements.Num());

				for (int32 ElementIndex = 0; ElementIndex < SrcLeaf.Elements.Num(); ElementIndex++)
				{
					const FVoxelAABBTree::FElement& SrcElement = SrcLeaf.Elements[ElementIndex];
					TPayloadBoundsElement<int32, float>& DestElement = DestLeaf.Elems[ElementIndex];

					DestElement.Payload = SrcElement.Payload;
					DestElement.Bounds = FAABB3f(SrcElement.Bounds.Min, SrcElement.Bounds.Max);
				}
			}

			VOXEL_SCOPE_COUNTER("FTriangleMeshImplicitObject::FTriangleMeshImplicitObject");
#if VOXEL_ENGINE_VERSION < 504
			return MakeVoxelShareable(new (GVoxelMemory) FTriangleMeshImplicitObject(
#else
			return (new FTriangleMeshImplicitObject(
#endif
				MoveTemp(Particles),
				MoveTemp(Triangles),
				TArray<uint16>(FaceMaterials),
				BVH,
				nullptr,
				nullptr,
				true));
		}
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMarchingCubeCollider::FVoxelMarchingCubeCollider(
	const FVector& Offset,
	const FVoxelBox& LocalBounds,
	const UE_504_SWITCH(TSharedRef, TRefCountPtr)<Chaos::FTriangleMeshImplicitObject> & TriangleMesh,
	const TVoxelArray<TWeakObjectPtr<UPhysicalMaterial>>& PhysicalMaterials)
	: Offset(Offset)
	, LocalBounds(LocalBounds)
	, TriangleMesh(TriangleMesh)
	, PhysicalMaterials(PhysicalMaterials)
{
}

FVoxelMarchingCubeCollider::~FVoxelMarchingCubeCollider()
{
}

int64 FVoxelMarchingCubeCollider::GetAllocatedSize() const
{
	int64 AllocatedSize = sizeof(*this);
	AllocatedSize += Chaos::FTriangleMeshSweepVisitorCCD<void, void>::GetAllocatedSize(*TriangleMesh);
	AllocatedSize += PhysicalMaterials.GetAllocatedSize();
	return AllocatedSize;
}

TSharedPtr<FVoxelMarchingCubeCollider> FVoxelMarchingCubeCollider::Create(
	const FVector& Offset,
	const TConstVoxelArrayView<int32> Indices,
	const TConstVoxelArrayView<FVector3f> Vertices,
	const TConstVoxelArrayView<uint16> FaceMaterials,
	TVoxelArray<TWeakObjectPtr<UPhysicalMaterial>>&& PhysicalMaterials)
{
	VOXEL_FUNCTION_COUNTER();

	if (Indices.Num() == 0 ||
		!ensure(Indices.Num() % 3 == 0))
	{
		return nullptr;
	}

	using FCooker = Chaos::FTriangleMeshOverlapVisitorNoMTD<Chaos::FCookTriangleDummy>;

	UE_504_SWITCH(TSharedPtr, TRefCountPtr)<Chaos::FTriangleMeshImplicitObject> TriangleMesh;
	if (Vertices.Num() < MAX_uint16)
	{
		TriangleMesh = FCooker::CookTriangleMesh<uint16>(Indices, Vertices, FaceMaterials);
	}
	else
	{
		TriangleMesh = FCooker::CookTriangleMesh<int32>(Indices, Vertices, FaceMaterials);
	}

	if (!TriangleMesh)
	{
		return nullptr;
	}

	return MakeVoxelShared<FVoxelMarchingCubeCollider>(
		Offset,
		FVoxelBox::FromPositions(Vertices),
		TriangleMesh UE_504_SWITCH(.ToSharedRef(),),
		MoveTemp(PhysicalMaterials));
}