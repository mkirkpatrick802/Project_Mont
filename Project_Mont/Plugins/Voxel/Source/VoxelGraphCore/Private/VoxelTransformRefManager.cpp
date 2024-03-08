﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTransformRefManager.h"

FVoxelTransformRefManager* GVoxelTransformRefManager = new FVoxelTransformRefManager();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelTransformRefImpl> FVoxelTransformRefManager::Make_AnyThread(const TConstVoxelArrayView<FVoxelTransformRefNode> Nodes)
{
	if (IsInGameThread())
	{
		return Make_GameThread(Nodes);
	}

	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	const FVoxelTransformRefNodeArray NodeArray(Nodes);

	if (const TSharedPtr<FVoxelTransformRefImpl> TransformRef = NodeArrayToWeakTransformRef_RequiresLock.FindRef(NodeArray).Pin())
	{
		return TransformRef.ToSharedRef();
	}

	const TSharedRef<FVoxelTransformRefImpl> TransformRef = MakeVoxelShareable(new (GVoxelMemory) FVoxelTransformRefImpl(Nodes));
	TransformRef->TryInitialize_AnyThread();
	NodeArrayToWeakTransformRef_RequiresLock.FindOrAdd(NodeArray) = TransformRef;

	for (const FVoxelTransformRefNode& Node : Nodes)
	{
		ComponentToWeakTransformRefs_RequiresLock.FindOrAdd(Node.WeakComponent).Add(TransformRef);
	}

	// Keep the transform ref alive for better reuse between tasks
	SharedTransformRefs_RequiresLock.Add(TransformRef);

	return TransformRef;
}

TSharedRef<FVoxelTransformRefImpl> FVoxelTransformRefManager::Make_GameThread(const TConstVoxelArrayView<FVoxelTransformRefNode> Nodes)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);
	check(IsInGameThread());

	const FVoxelTransformRefNodeArray NodeArray(Nodes);

	if (const TSharedPtr<FVoxelTransformRefImpl> TransformRef = NodeArrayToWeakTransformRef_RequiresLock.FindRef(NodeArray).Pin())
	{
		return TransformRef.ToSharedRef();
	}

	const TSharedRef<FVoxelTransformRefImpl> TransformRef = MakeVoxelShareable(new (GVoxelMemory) FVoxelTransformRefImpl(Nodes));
	TransformRef->Update_GameThread();
	NodeArrayToWeakTransformRef_RequiresLock.FindOrAdd(NodeArray) = TransformRef;

	for (const FVoxelTransformRefNode& Node : Nodes)
	{
		ComponentToWeakTransformRefs_RequiresLock.FindOrAdd(Node.WeakComponent).Add(TransformRef);
	}

	return TransformRef;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelTransformRefImpl> FVoxelTransformRefManager::Find_AnyThread_RequiresLock(const FVoxelTransformRefNodeArray& NodeArray) const
{
	checkVoxelSlow(CriticalSection.IsLocked());
	return NodeArrayToWeakTransformRef_RequiresLock.FindRef(NodeArray).Pin();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTransformRefManager::NotifyTransformChanged(const USceneComponent& Component)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);
	check(IsInGameThread());

	LOG_VOXEL(Verbose, "NotifyTransformChanged %s", *Component.GetReadableName());

	TVoxelSet<TWeakPtr<FVoxelTransformRefImpl>>* WeakTransformRefs = ComponentToWeakTransformRefs_RequiresLock.Find(&Component);
	if (!WeakTransformRefs)
	{
		return;
	}

	for (auto It = WeakTransformRefs->CreateIterator(); It; ++It)
	{
		const TSharedPtr<FVoxelTransformRefImpl> TransformRef = It->Pin();
		if (!TransformRef)
		{
			It.RemoveCurrent();
			continue;
		}

		TransformRef->Update_GameThread();
	}
}

void FVoxelTransformRefManager::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	const double Time = FPlatformTime::Seconds();
	if (LastClearTime + 10. > Time)
	{
		return;
	}
	LastClearTime = Time;

	VOXEL_SCOPE_LOCK(CriticalSection);
	SharedTransformRefs_RequiresLock.Reset();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelTransformRefManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);

	// Tricky: ResolveObjectPtr is not safe to check during GC

	for (auto It = ComponentToWeakTransformRefs_RequiresLock.CreateIterator(); It; ++It)
	{
		It.Value().Remove(nullptr);

		if (It.Value().Num() == 0)
		{
			It.RemoveCurrent();
		}
	}

	for (auto It = NodeArrayToWeakTransformRef_RequiresLock.CreateIterator(); It; ++It)
	{
		if (!It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}