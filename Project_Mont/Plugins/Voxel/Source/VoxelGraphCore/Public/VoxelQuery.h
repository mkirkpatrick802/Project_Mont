// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelCallstack.h"
#include "VoxelRuntimeInfo.h"
#include "VoxelQueryParameter.h"

struct FVoxelNode;
class FVoxelQueryCache;
class FVoxelDependencyTracker;
class FVoxelTerminalGraphInstance;

enum class EVoxelQueryInfo : uint8
{
	Query,
	Local
};

struct VOXELGRAPHCORE_API FVoxelSharedQueryCache
{
	mutable FVoxelCriticalSection CriticalSection;
	mutable TVoxelMap<TWeakPtr<const FVoxelTerminalGraphInstance>, TSharedPtr<FVoxelQueryCache>> TerminalGraphInstanceToQueryCache_RequiresLock;

	FVoxelSharedQueryCache();
	FVoxelQueryCache& GetQueryCache(const FVoxelTerminalGraphInstance& InTerminalGraphInstance) const;
};

struct VOXELGRAPHCORE_API FVoxelQueryImpl
{
	FVoxelQueryImpl(
		const TSharedRef<const FVoxelRuntimeInfo>& QueryRuntimeInfo,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		const TSharedRef<const FVoxelQueryParameters>& Parameters,
		const TSharedRef<FVoxelDependencyTracker>& DependencyTracker,
		const TSharedRef<const FVoxelSharedQueryCache>& SharedCache);

	UE_NONCOPYABLE(FVoxelQueryImpl);

	VOXEL_COUNT_INSTANCES();

	mutable FVoxelCounter32 NumReferences;

	// The RuntimeInfo of the object making this query
	// Typically if we're generating a voxel mesh this would be the voxel actor currently generating the mesh
	const TSharedRef<const FVoxelRuntimeInfo> QueryRuntimeInfo;
	// When querying a channel, we might end up with different root instances
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance;
	// The transient query parameters such as position, LOD etc
	const TSharedRef<const FVoxelQueryParameters> Parameters;
	const TSharedRef<FVoxelDependencyTracker> DependencyTracker;
	const TSharedRef<const FVoxelSharedQueryCache> SharedCache;

	FVoxelQueryCache& QueryCache;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelQuery
{
public:
	FORCEINLINE ~FVoxelQuery()
	{
		if (!Impl)
		{
			// Moved
			return;
		}

		if (Impl->NumReferences.Decrement_ReturnNew() > 0)
		{
			return;
		}

		FVoxelMemory::Delete(Impl);
	}

	FORCEINLINE FVoxelQuery(const FVoxelQuery& Other)
		: FVoxelQuery(Other.Impl, ToSharedRefFast(Other.Callstack))
	{
	}
	FORCEINLINE FVoxelQuery(FVoxelQuery&& Other)
		: Impl(Other.Impl)
		, Callstack(MoveTemp(Other.Callstack))
	{
		checkVoxelSlow(Impl);
		checkVoxelSlow(Callstack);

		Other.Impl = nullptr;
	}

	// Don't allow assigning a query ref
	FVoxelQuery& operator=(const FVoxelQuery& Other) = delete;
	FVoxelQuery& operator=(FVoxelQuery&& Other) = delete;

public:
	static FVoxelQuery Make(
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		const TSharedRef<const FVoxelQueryParameters>& Parameters,
		const TSharedRef<FVoxelDependencyTracker>& DependencyTracker);

	FVoxelQuery EnterScope(const FVoxelGraphNodeRef& Node, FName PinName = {}) const;
	FVoxelQuery EnterScope(const FVoxelNode& Node, FName PinName = {}) const;
	FVoxelQuery MakeNewQuery(const TSharedRef<const FVoxelQueryParameters>& NewParameters) const;
	FVoxelQuery MakeNewQuery(const TSharedRef<FVoxelTerminalGraphInstance>& NewTerminalGraphInstance) const;

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelQueryParameter>::Value>>
	FVoxelQuery MakeNewQuery(const TSharedRef<T>& NewParameter) const
	{
		const TSharedRef<FVoxelQueryParameters> NewParameters = CloneParameters();
		NewParameters->Add(NewParameter);
		return MakeNewQuery(NewParameters);
	}

public:
	FORCEINLINE FVoxelTerminalGraphInstance& GetTerminalGraphInstance() const
	{
		return Impl->TerminalGraphInstance.Get();
	}
	FORCEINLINE const FVoxelQueryParameters& GetParameters() const
	{
		return Impl->Parameters.Get();
	}
	FORCEINLINE FVoxelDependencyTracker& GetDependencyTracker() const
	{
		return Impl->DependencyTracker.Get();
	}

	FORCEINLINE const TSharedRef<FVoxelTerminalGraphInstance>& GetSharedTerminalGraphInstance() const
	{
		return Impl->TerminalGraphInstance;
	}
	FORCEINLINE const TSharedRef<const FVoxelQueryParameters>& GetSharedParameters() const
	{
		return Impl->Parameters;
	}
	FORCEINLINE const TSharedRef<FVoxelDependencyTracker>& GetSharedDependencyTracker() const
	{
		return Impl->DependencyTracker;
	}

	FORCEINLINE const FVoxelCallstack& GetCallstack() const
	{
		return *Callstack;
	}
	FORCEINLINE FVoxelQueryCache& GetQueryCache() const
	{
		checkVoxelSlow(&Impl->QueryCache == &Impl->SharedCache->GetQueryCache(*Impl->TerminalGraphInstance));
		return Impl->QueryCache;
	}

	const FVoxelRuntimeInfo& GetInfo(EVoxelQueryInfo Info) const;

public:
	FORCEINLINE FVoxelGraphNodeRef GetTopNode() const
	{
		return Callstack->GetNodeRef();
	}
	FORCEINLINE TSharedRef<FVoxelQueryParameters> CloneParameters() const
	{
		return GetParameters().Clone();
	}

public:
	FORCEINLINE FVoxelTransformRef GetLocalToWorld() const
	{
		return GetInfo(EVoxelQueryInfo::Local).GetLocalToWorld();
	}
	FORCEINLINE FVoxelTransformRef GetQueryToWorld() const
	{
		return GetInfo(EVoxelQueryInfo::Query).GetLocalToWorld();
	}

	FORCEINLINE FVoxelTransformRef GetLocalToQuery() const
	{
		return
			GetLocalToWorld() *
			GetQueryToWorld().Inverse();
	}
	FORCEINLINE FVoxelTransformRef GetQueryToLocal() const
	{
		return
			GetQueryToWorld() *
			GetLocalToWorld().Inverse();
	}

private:
	const FVoxelQueryImpl* Impl = nullptr;
	TSharedPtr<const FVoxelCallstack> Callstack;

	FORCEINLINE FVoxelQuery(
		const FVoxelQueryImpl* Impl,
		const TSharedRef<const FVoxelCallstack>& Callstack)
		: Impl(Impl)
		, Callstack(Callstack)
	{
		checkVoxelSlow(Impl);

		Impl->NumReferences.Increment();
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern VOXELGRAPHCORE_API const uint32 GVoxelQueryContextTLS;

class VOXELGRAPHCORE_API FVoxelQueryScope
{
public:
	const FVoxelQueryScope* const Parent;
	const FVoxelQuery* const Query;
	const FVoxelTerminalGraphInstance* const TerminalGraphInstance;

	FORCEINLINE FVoxelQueryScope(
		const FVoxelQuery* Query,
		const FVoxelTerminalGraphInstance* TerminalGraphInstance)
		: Parent(TryGet())
		, Query(Query)
		, TerminalGraphInstance(TerminalGraphInstance)
	{
		FPlatformTLS::SetTlsValue(GVoxelQueryContextTLS, this);
	}
	FORCEINLINE explicit FVoxelQueryScope(const FVoxelQuery& Query)
		: FVoxelQueryScope(&Query, &Query.GetTerminalGraphInstance())
	{
	}
	FORCEINLINE ~FVoxelQueryScope()
	{
		FPlatformTLS::SetTlsValue(GVoxelQueryContextTLS, const_cast<FVoxelQueryScope*>(Parent));
	}

	FORCEINLINE static const FVoxelQueryScope* TryGet()
	{
		return static_cast<const FVoxelQueryScope*>(FPlatformTLS::GetTlsValue(GVoxelQueryContextTLS));
	}
};