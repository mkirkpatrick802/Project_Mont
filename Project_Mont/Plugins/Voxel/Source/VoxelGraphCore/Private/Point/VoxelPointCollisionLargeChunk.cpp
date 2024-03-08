// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelPointCollisionLargeChunk.h"
#include "VoxelRuntime.h"
#include "VoxelNodeStats.h"
#include "Point/VoxelPointManager.h"
#include "Point/VoxelInstancedMeshComponentSettings.h"
#include "Point/VoxelInstancedCollisionComponent.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelPointCollisionLargeChunk);

FVoxelPointCollisionLargeChunk::FVoxelPointCollisionLargeChunk(
	const FVoxelGraphNodeRef& NodeRef,
	const FVoxelPointChunkRef& ChunkRef,
	const TVoxelDynamicValue<FVoxelPointSet>& PointsValue,
	const FVoxelNameBuffer& AttributesToCache)
	: NodeRef(NodeRef)
	, ChunkRef(ChunkRef)
	, PointsValue(PointsValue)
	, AttributesToCache(AttributesToCache)
{
}

void FVoxelPointCollisionLargeChunk::Initialize(const FObjectKey& World)
{
	PointsValue.OnChanged(MakeWeakPtrLambda(this, [this](const TSharedRef<const FVoxelPointSet>& NewPoints)
	{
		UpdatePoints(NewPoints);
	}));

	for (const FName Name : AttributesToCache)
	{
		FVoxelPointManager::Get(World)->RegisterFindPoint(
			Name,
			ChunkRef,
			MakeWeakPtrDelegate(this, [=](const FVoxelPointId PointId, FVoxelRuntimePinValue& Value)
			{
				VOXEL_SCOPE_LOCK(CriticalSection);

				const TSharedPtr<const FVoxelBuffer> Buffer = CachedAttributes_RequiresLock.FindRef(Name);
				if (!Buffer)
				{
					return;
				}

				const int32* IndexPtr = CachedAttributeIds_RequiresLock.Find(PointId);
				if (!IndexPtr ||
					!ensure(Buffer->IsValidIndex(*IndexPtr)))
				{
					return;
				}

				Value = Buffer->GetGeneric(*IndexPtr);
			}));
	}

	FVoxelPointManager::Get(World)->RegisterFindPoint(
		FVoxelPointAttributes::Position,
		ChunkRef,
		MakeWeakPtrDelegate(this, [=](const FVoxelPointId PointId, FVoxelRuntimePinValue& Value)
		{
			VOXEL_SCOPE_LOCK(CriticalSection);

			if (!MeshToPoints_RequiresLock)
			{
				return;
			}

			for (const auto& It : *MeshToPoints_RequiresLock)
			{
				if (const FPointTransform* Transform = It.Value.Find(PointId))
				{
					Value = FVoxelRuntimePinValue::Make<FVector>(FVector(
						Transform->PositionX,
						Transform->PositionY,
						Transform->PositionZ));
					return;
				}
			}
		}));

	FVoxelPointManager::Get(World)->RegisterFindPoint(
		FVoxelPointAttributes::Rotation,
		ChunkRef,
		MakeWeakPtrDelegate(this, [=](const FVoxelPointId PointId, FVoxelRuntimePinValue& Value)
		{
			VOXEL_SCOPE_LOCK(CriticalSection);

			if (!MeshToPoints_RequiresLock)
			{
				return;
			}

			for (const auto& It : *MeshToPoints_RequiresLock)
			{
				if (const FPointTransform* Transform = It.Value.Find(PointId))
				{
					Value = FVoxelRuntimePinValue::Make<FQuat>(FQuat(
						Transform->RotationX,
						Transform->RotationY,
						Transform->RotationZ,
						Transform->RotationW));
					return;
				}
			}
		}));

	FVoxelPointManager::Get(World)->RegisterFindPoint(
		FVoxelPointAttributes::Scale,
		ChunkRef,
		MakeWeakPtrDelegate(this, [=](const FVoxelPointId PointId, FVoxelRuntimePinValue& Value)
		{
			VOXEL_SCOPE_LOCK(CriticalSection);

			if (!MeshToPoints_RequiresLock)
			{
				return;
			}

			for (const auto& It : *MeshToPoints_RequiresLock)
			{
				if (const FPointTransform* Transform = It.Value.Find(PointId))
				{
					Value = FVoxelRuntimePinValue::Make<FVector>(FVector(
						Transform->ScaleX,
						Transform->ScaleY,
						Transform->ScaleZ));
					return;
				}
			}
		}));
}

void FVoxelPointCollisionLargeChunk::AddOnChanged(const FOnChanged& OnChanged)
{
	VOXEL_FUNCTION_COUNTER();

	TSharedPtr<const FMeshToPoints> MeshToPoints;
	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		OnChanged_RequiresLock.Add(OnChanged);
		MeshToPoints = MeshToPoints_RequiresLock;
	}

	if (MeshToPoints)
	{
		OnChanged(*MeshToPoints);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelPointCollisionLargeChunk::UpdatePoints(const TSharedRef<const FVoxelPointSet>& NewPoints)
{
	VOXEL_SCOPE_COUNTER_FORMAT("FVoxelPointCollisionLargeChunk::UpdatePoints Num=%d", NewPoints->Num());
	ensure(!IsInGameThread() || GVoxelBypassTaskQueue);
	FVoxelNodeStatScope StatScope(*this, NewPoints->Num());

	FindVoxelPointSetAttributeVoid(*NewPoints, FVoxelPointAttributes::Id, FVoxelPointIdBuffer, NewIds);
	FindVoxelPointSetAttributeVoid(*NewPoints, FVoxelPointAttributes::Mesh, FVoxelStaticMeshBuffer, NewMeshes);
	FindVoxelPointSetAttributeVoid(*NewPoints, FVoxelPointAttributes::Position, FVoxelVectorBuffer, NewPositions);
	FindVoxelPointSetOptionalAttribute(*NewPoints, FVoxelPointAttributes::Rotation, FVoxelQuaternionBuffer, NewRotations, FQuat::Identity);
	FindVoxelPointSetOptionalAttribute(*NewPoints, FVoxelPointAttributes::Scale, FVoxelVectorBuffer, NewScales, FVector::OneVector);

	const TSharedRef<FMeshToPoints> MeshToPoints = MakeVoxelShared<FMeshToPoints>();

	ON_SCOPE_EXIT
	{
		TVoxelArray<FOnChanged> OnChangedCopy;
		{
			VOXEL_SCOPE_LOCK(CriticalSection);
			MeshToPoints_RequiresLock = MeshToPoints;
			OnChangedCopy = OnChanged_RequiresLock;
		}

		VOXEL_SCOPE_COUNTER("OnChanged");

		for (const FOnChanged& OnChanged : OnChangedCopy)
		{
			OnChanged(*MeshToPoints);
		}
	};

	if (AttributesToCache.Num() > 0)
	{
		CachedAttributeIds_RequiresLock.Reset();
		CachedAttributeIds_RequiresLock.Reserve(NewPoints->Num());
	}

	for (int32 NewIndex = 0; NewIndex < NewPoints->Num(); NewIndex++)
	{
		const FVoxelStaticMesh Mesh = NewMeshes[NewIndex];

		TVoxelMap<FVoxelPointId, FPointTransform>& Points = MeshToPoints->FindOrAdd(Mesh);
		if (Points.Num() == 0)
		{
			VOXEL_SCOPE_COUNTER("Reserve");
			Points.Reserve(NewPoints->Num());
		}

		const FVoxelPointId PointId = NewIds[NewIndex];
		const uint32 Hash = Points.HashValue(PointId);

		if (Points.FindHashed(Hash, PointId))
		{
			VOXEL_MESSAGE(Error, "{0}: Duplicated point ids", this);
			continue;
		}

		if (AttributesToCache.Num() > 0)
		{
			CachedAttributeIds_RequiresLock.AddHashed_CheckNew_NoRehash(Hash, PointId) = NewIndex;
		}

		FPointTransform& Transform = Points.AddHashed_CheckNew_NoRehash(Hash, PointId);

		Transform.PositionX = NewPositions.X[NewIndex];
		Transform.PositionY = NewPositions.Y[NewIndex];
		Transform.PositionZ = NewPositions.Z[NewIndex];

		Transform.ScaleX = NewScales.X[NewIndex];
		Transform.ScaleY = NewScales.Y[NewIndex];
		Transform.ScaleZ = NewScales.Z[NewIndex];

		Transform.RotationX = NewRotations.X[NewIndex];
		Transform.RotationY = NewRotations.Y[NewIndex];
		Transform.RotationZ = NewRotations.Z[NewIndex];
		Transform.RotationW = NewRotations.W[NewIndex];
	}

	CachedAttributes_RequiresLock.Reset();

	for (const FName Name : AttributesToCache)
	{
		if (const TSharedPtr<const FVoxelBuffer> Buffer = NewPoints->Find(Name))
		{
			CachedAttributes_RequiresLock.Add_EnsureNew(Name, Buffer);
		}
	}
}