// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphEvaluator.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphCompiler.h"
#include "VoxelTerminalGraph.h"
#include "VoxelFutureValueState.h"
#include "VoxelDependencyTracker.h"
#include "Nodes/VoxelNode_Root.h"

FVoxelGraphEvaluatorManager* GVoxelGraphEvaluatorManager = new FVoxelGraphEvaluatorManager();

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelGraphEvaluator);
DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelGraphEvaluatorRef);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelGraphEvaluator> FVoxelGraphEvaluator::Create(
	const FOnVoxelGraphChanged& OnChanged,
	const FVoxelGraphPinRef& GraphPinRef)
{
	VOXEL_FUNCTION_COUNTER();

	const UVoxelTerminalGraph* TerminalGraph = GraphPinRef.GetNodeTerminalGraph(OnChanged);
	if (!TerminalGraph)
	{
		// Output with no output node placed
		return MakeVoxelShared<FVoxelGraphEvaluator>();
	}

#if WITH_EDITOR
	GVoxelGraphTracker->OnTerminalGraphCompiled(*TerminalGraph).Add(OnChanged);
#endif

	return TerminalGraph->GetRuntime().CreateEvaluator(GraphPinRef);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphEvaluator::Initialize(
	const FName NewRootPinName,
	const TSharedRef<const FVoxelNode_Root>& NewRootNode,
	TVoxelSet<TSharedPtr<const FVoxelNode>>&& NewNodes,
	const TSharedRef<const Voxel::Graph::FGraph>& NewGraph)
{
	RootPinName = NewRootPinName;
	RootNode = NewRootNode;
	Nodes = MoveTemp(NewNodes);
#if WITH_EDITOR
	Graph_EditorOnly = NewGraph->Clone();
#endif
}

FVoxelFutureValue FVoxelGraphEvaluator::Get(const FVoxelQuery& Query) const
{
	if (!RootNode)
	{
		// Empty graph
		return {};
	}
	return RootNode->GetNodeRuntime().Get(FVoxelPinRef(RootPinName), Query);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelGraphEvaluatorRef> FVoxelGraphEvaluatorRef::Create(const FVoxelGraphPinRef& GraphPinRef)
{
	const TSharedRef<FVoxelGraphEvaluatorRef> EvaluatorRef = MakeVoxelShareable(new (GVoxelMemory) FVoxelGraphEvaluatorRef(GraphPinRef));
	EvaluatorRef->Evaluator_RequiresLock = EvaluatorRef->CreateNewEvaluator();
	return EvaluatorRef;
}

TSharedPtr<const FVoxelGraphEvaluator> FVoxelGraphEvaluatorRef::GetEvaluator_NoDependency() const
{
	VOXEL_SCOPE_LOCK(CriticalSection);
	return Evaluator_RequiresLock;
}

TSharedPtr<const FVoxelGraphEvaluator> FVoxelGraphEvaluatorRef::GetEvaluator(const FVoxelQuery& Query) const
{
	// Always add dependency, especially if we failed to compile
	Query.GetDependencyTracker().AddDependency(Dependency);

	return GetEvaluator_NoDependency();
}

FVoxelFutureValue FVoxelGraphEvaluatorRef::Compute(const FVoxelQuery& Query) const
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<const FVoxelGraphEvaluator> Evaluator = GetEvaluator(Query);
	if (!Evaluator)
	{
		// Failed to compile
		return {};
	}

	FVoxelTaskReferencer::Get().AddEvaluator(Evaluator.ToSharedRef());
	return Evaluator->Get(Query);
}

#if WITH_EDITOR
void FVoxelGraphEvaluatorRef::Recompile()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	if (GraphPinRef.NodeRef.TerminalGraphRef.Graph.IsStale())
	{
		// Asset is being deleted
		return;
	}

	if (GraphPinRef.NodeRef.IsDeleted())
	{
		LOG_VOXEL(Verbose, "%s deleted", *GraphPinRef.ToString());
		{
			VOXEL_SCOPE_LOCK(CriticalSection);
			Evaluator_RequiresLock.Reset();
		}
		Dependency->Invalidate();
		return;
	}

	LOG_VOXEL(Verbose, "Recompiling %s", *GraphPinRef.ToString());

	const TSharedPtr<const FVoxelGraphEvaluator> NewEvaluator = CreateNewEvaluator();
	const TSharedPtr<const FVoxelGraphEvaluator> OldEvaluator = GetEvaluator_NoDependency();

	const bool bHasChanged = INLINE_LAMBDA
	{
		if (NewEvaluator.IsValid() != OldEvaluator.IsValid())
		{
			LOG_VOXEL(Verbose, "Updating %s: compilation status changed", *GraphPinRef.ToString());
			return true;
		}

		if (!NewEvaluator.IsValid())
		{
			// Failed to compile
			ensure(!OldEvaluator.IsValid());
			return false;
		}

		if (NewEvaluator->GetGraph_EditorOnly().IsValid() != OldEvaluator->GetGraph_EditorOnly().IsValid())
		{
			LOG_VOXEL(Verbose, "Updating %s: IsEmpty changed", *GraphPinRef.ToString());
			return true;
		}

		if (!NewEvaluator->GetGraph_EditorOnly().IsValid())
		{
			// Failed to compile
			ensure(!OldEvaluator->GetGraph_EditorOnly().IsValid());
			return false;
		}

		FString Diff;
		if (!OldEvaluator->GetGraph_EditorOnly()->Identical(*NewEvaluator->GetGraph_EditorOnly(), &Diff))
		{
			LOG_VOXEL(Verbose, "Updating %s: %s", *GraphPinRef.ToString(), *Diff);
			return true;
		}

		return false;
	};

	if (!bHasChanged)
	{
		return;
	}

	{
		VOXEL_SCOPE_LOCK(CriticalSection);
		Evaluator_RequiresLock = NewEvaluator;
	}
	Dependency->Invalidate();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelGraphEvaluatorRef::FVoxelGraphEvaluatorRef(const FVoxelGraphPinRef& GraphPinRef)
	: GraphPinRef(GraphPinRef)
	, Dependency(FVoxelDependency::Create(STATIC_FNAME("GraphEvaluator"), *GraphPinRef.ToString()))
{
}

TSharedPtr<FVoxelGraphEvaluator> FVoxelGraphEvaluatorRef::CreateNewEvaluator()
{
	VOXEL_FUNCTION_COUNTER();

#if WITH_EDITOR
	OnChangedPtr = MakeSharedVoid();
#endif

	const TSharedPtr<FVoxelGraphEvaluator> Evaluator = FVoxelGraphEvaluator::Create(
#if WITH_EDITOR
		FOnVoxelGraphChanged::Make(OnChangedPtr, this, [=]
		{
			GVoxelGraphEvaluatorManager->Recompile(AsShared());
		}),
#else
		FOnVoxelGraphChanged::Null(),
#endif
		GraphPinRef);

	if (!Evaluator)
	{
		VOXEL_MESSAGE(Error, "{0} failed to compile", GraphPinRef);
	}

	return Evaluator;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void FVoxelGraphEvaluatorManager::Recompile(const TSharedRef<FVoxelGraphEvaluatorRef>& EvaluatorRef)
{
	check(IsInGameThread());
	EvaluatorRefsToRecompile_GameThread.Add(EvaluatorRef);
}
#endif

TSharedRef<FVoxelGraphEvaluatorRef> FVoxelGraphEvaluatorManager::MakeEvaluatorRef_GameThread(const FVoxelGraphPinRef& GraphPinRef)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (const TSharedPtr<FVoxelGraphEvaluatorRef> EvaluatorRef = INLINE_LAMBDA
		{
			VOXEL_SCOPE_LOCK(CriticalSection);
			return GraphPinRefToWeakEvaluatorRef_RequiresLock.FindRef(GraphPinRef).Pin();
		})
	{
		return EvaluatorRef.ToSharedRef();
	}

	const TSharedRef<FVoxelGraphEvaluatorRef> EvaluatorRef = FVoxelGraphEvaluatorRef::Create(GraphPinRef);

	VOXEL_SCOPE_LOCK(CriticalSection);
	GraphPinRefToWeakEvaluatorRef_RequiresLock.FindOrAdd(GraphPinRef) = EvaluatorRef;
	return EvaluatorRef;
}

TVoxelFutureValue<FVoxelGraphEvaluatorRefWrapper> FVoxelGraphEvaluatorManager::MakeEvaluatorRef_AnyThread(const FVoxelGraphPinRef& GraphPinRef)
{
	VOXEL_FUNCTION_COUNTER();

	if (const TSharedPtr<FVoxelGraphEvaluatorRef> EvaluatorRef = INLINE_LAMBDA
		{
			VOXEL_SCOPE_LOCK(CriticalSection);
			return GraphPinRefToWeakEvaluatorRef_RequiresLock.FindRef(GraphPinRef).Pin();
		})
	{
		return FVoxelGraphEvaluatorRefWrapper
		{
			EvaluatorRef.ToSharedRef()
		};
	}

	return
		MakeVoxelTask()
		.RunOnGameThread()
		.Execute<FVoxelGraphEvaluatorRefWrapper>([=]
		{
			return FVoxelGraphEvaluatorRefWrapper
			{
				MakeEvaluatorRef_GameThread(GraphPinRef)
			};
		});
}

void FVoxelGraphEvaluatorManager::Tick()
{
	VOXEL_FUNCTION_COUNTER();

#if WITH_EDITOR
	// Group all the invalidate calls
	const FVoxelDependencyInvalidationScope DependencyInvalidationScope;

	for (const TSharedPtr<FVoxelGraphEvaluatorRef>& EvaluatorRef : EvaluatorRefsToRecompile_GameThread)
	{
		EvaluatorRef->Recompile();
	}
	EvaluatorRefsToRecompile_GameThread.Reset();
#endif
}