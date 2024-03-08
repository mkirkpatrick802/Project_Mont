// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"
#include "VoxelGraphNodeRef.h"
#include "VoxelGraphEvaluator.generated.h"

struct FVoxelNode;
struct FVoxelNode_Root;
class FVoxelDependency;

namespace Voxel::Graph
{
	class FGraph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Will keep pin default value objects alive
class VOXELGRAPHCORE_API FVoxelGraphEvaluator
{
public:
	static TSharedPtr<FVoxelGraphEvaluator> Create(
		const FOnVoxelGraphChanged& OnChanged,
		const FVoxelGraphPinRef& GraphPinRef);

public:
	VOXEL_COUNT_INSTANCES();

	FVoxelGraphEvaluator() = default;

	void Initialize(
		FName NewRootPinName,
		const TSharedRef<const FVoxelNode_Root>& NewRootNode,
		TVoxelSet<TSharedPtr<const FVoxelNode>>&& NewNodes,
		const TSharedRef<const Voxel::Graph::FGraph>& NewGraph);

public:
	FORCEINLINE bool IsDefaultValue() const
	{
		ensureVoxelSlow(Nodes.Num() > 0);
		return Nodes.Num() == 1;
	}
	FORCEINLINE bool HasNode(const FVoxelNode& Node) const
	{
		return Nodes.Contains(GetTypeHash(&Node), [&](const TSharedPtr<const FVoxelNode>& OtherNode)
		{
			return OtherNode.Get() == &Node;
		});
	}
#if WITH_EDITOR
	FORCEINLINE const TSharedPtr<const Voxel::Graph::FGraph>& GetGraph_EditorOnly() const
	{
		return Graph_EditorOnly;
	}
#endif

	FVoxelFutureValue Get(const FVoxelQuery& Query) const;

private:
	FName RootPinName;
	TSharedPtr<const FVoxelNode_Root> RootNode;
	TVoxelSet<TSharedPtr<const FVoxelNode>> Nodes;
#if WITH_EDITOR
	TSharedPtr<const Voxel::Graph::FGraph> Graph_EditorOnly;
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelGraphEvaluatorRef : public TSharedFromThis<FVoxelGraphEvaluatorRef>
{
public:
	const FVoxelGraphPinRef GraphPinRef;
	const TSharedRef<FVoxelDependency> Dependency;

	VOXEL_COUNT_INSTANCES();

	static TSharedRef<FVoxelGraphEvaluatorRef> Create(const FVoxelGraphPinRef& GraphPinRef);

public:
	TSharedPtr<const FVoxelGraphEvaluator> GetEvaluator_NoDependency() const;
	TSharedPtr<const FVoxelGraphEvaluator> GetEvaluator(const FVoxelQuery& Query) const;

	FVoxelFutureValue Compute(const FVoxelQuery& Query) const;

#if WITH_EDITOR
	void Recompile();
#endif

private:
#if WITH_EDITOR
	FSharedVoidPtr OnChangedPtr;
#endif
	mutable FVoxelCriticalSection CriticalSection;
	TSharedPtr<const FVoxelGraphEvaluator> Evaluator_RequiresLock;

	explicit FVoxelGraphEvaluatorRef(const FVoxelGraphPinRef& GraphPinRef);

	TSharedPtr<FVoxelGraphEvaluator> CreateNewEvaluator();
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphEvaluatorRefWrapper
{
	GENERATED_BODY()

	TSharedPtr<const FVoxelGraphEvaluatorRef> EvaluatorRef;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelGraphEvaluatorManager : public FVoxelSingleton
{
public:
#if WITH_EDITOR
	void Recompile(const TSharedRef<FVoxelGraphEvaluatorRef>& EvaluatorRef);
#endif

	TSharedRef<FVoxelGraphEvaluatorRef> MakeEvaluatorRef_GameThread(const FVoxelGraphPinRef& GraphPinRef);
	TVoxelFutureValue<FVoxelGraphEvaluatorRefWrapper> MakeEvaluatorRef_AnyThread(const FVoxelGraphPinRef& GraphPinRef);

	//~ Begin FVoxelSingleton Interface
	virtual void Tick() override;
	//~ End FVoxelSingleton Interface

private:
#if WITH_EDITOR
	TVoxelSet<TSharedPtr<FVoxelGraphEvaluatorRef>> EvaluatorRefsToRecompile_GameThread;
#endif

	FVoxelCriticalSection CriticalSection;
	TVoxelMap<FVoxelGraphPinRef, TWeakPtr<FVoxelGraphEvaluatorRef>> GraphPinRefToWeakEvaluatorRef_RequiresLock;
};
extern VOXELGRAPHCORE_API FVoxelGraphEvaluatorManager* GVoxelGraphEvaluatorManager;