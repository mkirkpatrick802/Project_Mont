// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelQuery.h"
#include "VoxelQueryCache.h"
#include "VoxelRuntimeInfo.h"
#include "VoxelTerminalGraphInstance.h"

const uint32 GVoxelQueryContextTLS = FPlatformTLS::AllocTlsSlot();

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelQueryImpl);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelSharedQueryCache::FVoxelSharedQueryCache()
{
	TerminalGraphInstanceToQueryCache_RequiresLock.Reserve(8);
}

FVoxelQueryCache& FVoxelSharedQueryCache::GetQueryCache(const FVoxelTerminalGraphInstance& InTerminalGraphInstance) const
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	TSharedPtr<FVoxelQueryCache>& Cache = TerminalGraphInstanceToQueryCache_RequiresLock.FindOrAdd(InTerminalGraphInstance.AsWeak());
	if (!Cache)
	{
		Cache = MakeVoxelShared<FVoxelQueryCache>();
	}
	return *Cache;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelQueryImpl::FVoxelQueryImpl(
	const TSharedRef<const FVoxelRuntimeInfo>& QueryRuntimeInfo,
	const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
	const TSharedRef<const FVoxelQueryParameters>& Parameters,
	const TSharedRef<FVoxelDependencyTracker>& DependencyTracker,
	const TSharedRef<const FVoxelSharedQueryCache>& SharedCache)
	: QueryRuntimeInfo(QueryRuntimeInfo)
	, TerminalGraphInstance(TerminalGraphInstance)
	, Parameters(Parameters)
	, DependencyTracker(DependencyTracker)
	, SharedCache(SharedCache)
	, QueryCache(SharedCache->GetQueryCache(*TerminalGraphInstance))
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelQuery FVoxelQuery::Make(
	const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
	const TSharedRef<const FVoxelQueryParameters>& Parameters,
	const TSharedRef<FVoxelDependencyTracker>& DependencyTracker)
{
	const FVoxelQueryImpl* Impl = new (GVoxelMemory) FVoxelQueryImpl(
		TerminalGraphInstance->RuntimeInfo,
		TerminalGraphInstance,
		Parameters,
		DependencyTracker,
		MakeVoxelShared<FVoxelSharedQueryCache>());

	return FVoxelQuery(Impl, TerminalGraphInstance->Callstack);
}

FVoxelQuery FVoxelQuery::EnterScope(const FVoxelGraphNodeRef& Node, const FName PinName) const
{
	const TSharedRef<const FVoxelCallstack> ChildCallstack = Callstack->MakeChild(
		PinName,
		Node,
		GetInfo(EVoxelQueryInfo::Local).GetRuntimeProvider().GetWeakObjectPtr());

	return FVoxelQuery(Impl, ChildCallstack);
}

FVoxelQuery FVoxelQuery::EnterScope(const FVoxelNode& Node, const FName PinName) const
{
	return EnterScope(Node.GetNodeRef(), PinName);
}

FVoxelQuery FVoxelQuery::MakeNewQuery(const TSharedRef<const FVoxelQueryParameters>& NewParameters) const
{
	const FVoxelQueryImpl* NewImpl = new (GVoxelMemory) FVoxelQueryImpl(
		Impl->QueryRuntimeInfo,
		Impl->TerminalGraphInstance,
		NewParameters,
		Impl->DependencyTracker,
		// New parameters, invalidate cache
		MakeVoxelShared<FVoxelSharedQueryCache>());

	return FVoxelQuery(NewImpl, Callstack.ToSharedRef());
}

FVoxelQuery FVoxelQuery::MakeNewQuery(const TSharedRef<FVoxelTerminalGraphInstance>& NewTerminalGraphInstance) const
{
	const FVoxelQueryImpl* NewImpl = new (GVoxelMemory) FVoxelQueryImpl(
		Impl->QueryRuntimeInfo,
		NewTerminalGraphInstance,
		Impl->Parameters,
		Impl->DependencyTracker,
		Impl->SharedCache);

	return FVoxelQuery(NewImpl, Callstack.ToSharedRef());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FVoxelRuntimeInfo& FVoxelQuery::GetInfo(const EVoxelQueryInfo Info) const
{
	if (Info == EVoxelQueryInfo::Query)
	{
		return *Impl->QueryRuntimeInfo;
	}
	else
	{
		checkVoxelSlow(Info == EVoxelQueryInfo::Local);
		return *Impl->TerminalGraphInstance->RuntimeInfo;
	}
}