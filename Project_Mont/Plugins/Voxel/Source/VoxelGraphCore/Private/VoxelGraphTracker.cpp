// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphTracker.h"
#include "VoxelGraph.h"
#include "VoxelTerminalGraph.h"
#include "VoxelTerminalGraphRuntime.h"
#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#endif

#if WITH_EDITOR
FVoxelGraphTracker* GVoxelGraphTracker = new FVoxelGraphTracker();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphTracker::FDelegateGroup::Add(const FOnVoxelGraphChanged& OnChanged)
{
	if (OnChanged.Delegate)
	{
		Delegates.Add(OnChanged.Delegate);
	}
}

void FVoxelGraphTracker::FImpl::Broadcast(const UObject& Object)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	if (GVoxelGraphTracker->bIsMuted)
	{
		return;
	}

	FDelegateGroup* DelegateGroup =
		&Object == &MakeAny<UObject>()
		? &AnyDelegateGroup
		: ObjectToDelegateGroup.Find(&Object);

	if (!DelegateGroup)
	{
		return;
	}

	for (auto It = DelegateGroup->Delegates.CreateIterator(); It; ++It)
	{
		if (!It->Get()->IsBound())
		{
			It.RemoveCurrentSwap();
			continue;
		}

		GVoxelGraphTracker->QueuedDelegates.Add(*It);
	}
}

FVoxelGraphTracker::FDelegateGroup& FVoxelGraphTracker::FImpl::Get(const UObject& Object)
{
	check(IsInGameThread());

	if (&Object == &MakeAny<UObject>())
	{
		return AnyDelegateGroup;
	}

	if (FDelegateGroup* DelegateGroup = ObjectToDelegateGroup.Find(&Object))
	{
		return *DelegateGroup;
	}

	VOXEL_FUNCTION_COUNTER();

	// Cleanup map
	for (auto It = ObjectToDelegateGroup.CreateIterator(); It; ++It)
	{
		if (!It.Key().ResolveObjectPtr())
		{
			It.RemoveCurrent();
		}
	}

	return ObjectToDelegateGroup.Add_CheckNew(&Object);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphTracker::Mute()
{
	ensure(!bIsMuted);
	bIsMuted = true;
}

void FVoxelGraphTracker::Unmute()
{
	ensure(bIsMuted);
	bIsMuted = false;
}

void FVoxelGraphTracker::Flush()
{
	VOXEL_FUNCTION_COUNTER();

	int32 NumIterations = 0;

	// Loop to avoid adding latency when delegates add other delegates
	while (QueuedDelegates.Num() > 0)
	{
		NumIterations++;

		if (NumIterations > 10)
		{
			ensure(false);
			VOXEL_MESSAGE(Error, "FVoxelGraphTracker is in an infinite loop, you might want to restart the engine");
			return;
		}

		const TVoxelSet<TSharedPtr<const FSimpleDelegate>> Delegates = MoveTemp(QueuedDelegates);
		ensure(QueuedDelegates.Num() == 0);

		for (const TSharedPtr<const FSimpleDelegate>& Delegate : Delegates)
		{
			(void)Delegate->ExecuteIfBound();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphTracker::NotifyEdGraphChanged(const UEdGraph& EdGraph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyEdGraphChanged %s", *EdGraph.GetPathName());

	if (const UVoxelTerminalGraph* TerminalGraph = EdGraph.GetTypedOuter<UVoxelTerminalGraph>())
	{
		// Hacky but there's no other place to do this
		TerminalGraph->GetRuntime().BindOnEdGraphChanged();
	}

	OnEdGraphChangedImpl.Broadcast(EdGraph);
}

void FVoxelGraphTracker::NotifyEdGraphNodeChanged(const UEdGraphNode& EdGraphNode)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyEdGraphNodeChanged %s", *EdGraphNode.GetPathName());

	const UEdGraph* EdGraph = EdGraphNode.GetGraph();
	if (!ensure(EdGraph))
	{
		return;
	}

	NotifyEdGraphChanged(*EdGraph);
}

void FVoxelGraphTracker::NotifyTerminalGraphTranslated(const UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyTerminalGraphTranslated %s %s", *TerminalGraph.GetDisplayName(), *TerminalGraph.GetPathName());

	OnTerminalGraphTranslatedImpl.Broadcast(TerminalGraph);
}

void FVoxelGraphTracker::NotifyTerminalGraphCompiled(const UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyTerminalGraphCompiled %s %s", *TerminalGraph.GetDisplayName(), *TerminalGraph.GetPathName());

	OnTerminalGraphCompiledImpl.Broadcast(TerminalGraph);
}

void FVoxelGraphTracker::NotifyTerminalGraphMetaDataChanged(const UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyTerminalGraphMetaDataChanged %s %s", *TerminalGraph.GetDisplayName(), *TerminalGraph.GetPathName());

	OnTerminalGraphMetaDataChangedImpl.Broadcast(TerminalGraph);
}

void FVoxelGraphTracker::NotifyBaseGraphChanged(UVoxelGraph& Graph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyBaseGraphChanged %s", *Graph.GetPathName());

	for (UVoxelGraph* ChildGraph : Graph.GetChildGraphs_LoadedOnly())
	{
		ChildGraph->Fixup();
		OnTerminalGraphChangedImpl.Broadcast(*ChildGraph);
		OnParameterChangedImpl.Broadcast(*ChildGraph);

		ChildGraph->ForeachTerminalGraph_NoInheritance([&](UVoxelTerminalGraph& TerminalGraph)
		{
			TerminalGraph.Fixup();
			OnInputChangedImpl.Broadcast(TerminalGraph);
			OnOutputChangedImpl.Broadcast(TerminalGraph);
			// Don't broadcast local variables, they're not inherited
		});
	}
}

void FVoxelGraphTracker::NotifyTerminalGraphChanged(UVoxelGraph& Graph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyTerminalGraphChanged %s", *Graph.GetPathName());

	// Update inputs & outputs of all child terminal graphs
	// We might have added a new terminal graph override, changing inherited inputs & outputs
	for (UVoxelGraph* ChildGraph : Graph.GetChildGraphs_LoadedOnly())
	{
		ChildGraph->Fixup();
		OnTerminalGraphChangedImpl.Broadcast(*ChildGraph);

		ChildGraph->ForeachTerminalGraph_NoInheritance([&](UVoxelTerminalGraph& TerminalGraph)
		{
			TerminalGraph.Fixup();
			OnInputChangedImpl.Broadcast(TerminalGraph);
			OnOutputChangedImpl.Broadcast(TerminalGraph);
		});
	}
}

void FVoxelGraphTracker::NotifyParameterChanged(UVoxelGraph& Graph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyParameterChanged %s", *Graph.GetPathName());

	for (UVoxelGraph* ChildGraph : Graph.GetChildGraphs_LoadedOnly())
	{
		ChildGraph->Fixup();
		OnParameterChangedImpl.Broadcast(*ChildGraph);
	}
}

void FVoxelGraphTracker::NotifyInputChanged(UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyInputChanged %s", *TerminalGraph.GetPathName());

	for (UVoxelTerminalGraph* ChildTerminalGraph : TerminalGraph.GetChildTerminalGraphs_LoadedOnly())
	{
		ChildTerminalGraph->Fixup();
		OnInputChangedImpl.Broadcast(*ChildTerminalGraph);
	}
}

void FVoxelGraphTracker::NotifyOutputChanged(UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyOutputChanged %s", *TerminalGraph.GetPathName());

	for (UVoxelTerminalGraph* ChildTerminalGraph : TerminalGraph.GetChildTerminalGraphs_LoadedOnly())
	{
		ChildTerminalGraph->Fixup();
		OnOutputChangedImpl.Broadcast(*ChildTerminalGraph);
	}
}

void FVoxelGraphTracker::NotifyLocalVariableChanged(UVoxelTerminalGraph& TerminalGraph)
{
	VOXEL_FUNCTION_COUNTER();
	LOG_VOXEL(Verbose, "NotifyLocalVariableChanged %s", *TerminalGraph.GetPathName());

	// No inheritance for local variables
	TerminalGraph.Fixup();
	OnLocalVariableChangedImpl.Broadcast(TerminalGraph);
}

///()////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphTracker::Tick()
{
	Flush();
}
#endif