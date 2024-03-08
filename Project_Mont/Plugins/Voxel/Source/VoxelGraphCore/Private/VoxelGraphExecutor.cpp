// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphExecutor.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelTerminalGraphRuntime.h"
#include "VoxelExecNodeRuntimeWrapper.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelGraphExecutor);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelGraphExecutor> FVoxelGraphExecutor::Create_GameThread(
	const FVoxelTerminalGraphRef& TerminalGraphRef,
	const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance)
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const TSharedRef<FVoxelGraphExecutor> Executor = MakeVoxelShareable(new (GVoxelMemory) FVoxelGraphExecutor(TerminalGraphRef, TerminalGraphInstance));
	Executor->Update_GameThread();
	return Executor;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphExecutor::Tick(FVoxelRuntime& Runtime) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	for (const auto& It : ExecNodeKeyToWrapper_GameThread)
	{
		It.Value->Tick(Runtime);
	}
}

void FVoxelGraphExecutor::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();

	for (const auto& It : ExecNodeKeyToWrapper_GameThread)
	{
		It.Value->AddReferencedObjects(Collector);
	}
}

FVoxelOptionalBox FVoxelGraphExecutor::GetBounds() const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelOptionalBox Result;
	for (const auto& It : ExecNodeKeyToWrapper_GameThread)
	{
		Result += It.Value->GetBounds();
	}
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphExecutor::Update_GameThread()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

#if WITH_EDITOR
	OnChangedPtr = MakeSharedVoid();

	const FOnVoxelGraphChanged OnChanged = FOnVoxelGraphChanged::Make(OnChangedPtr, this, [=]
	{
		Update_GameThread();
	});
#else
	const FOnVoxelGraphChanged OnChanged = FOnVoxelGraphChanged::Null();
#endif

	TVoxelSet<TSharedPtr<FVoxelExecNodeRuntimeWrapper>> WrappersToKeep;
	ON_SCOPE_EXIT
	{
		for (auto It = ExecNodeKeyToWrapper_GameThread.CreateIterator(); It; ++It)
		{
			if (!WrappersToKeep.Contains(It.Value()))
			{
				It.RemoveCurrent();
			}
		}
	};

	const auto AddTerminalGraph = [&](const UVoxelTerminalGraph& TerminalGraph)
	{
#if WITH_EDITOR
		GVoxelGraphTracker->OnTerminalGraphTranslated(TerminalGraph).Add(OnChanged);
#endif

		for (const auto& NodeIt : TerminalGraph.GetRuntime().GetSerializedGraph().NodeNameToNode)
		{
			const FVoxelSerializedNode& SerializedNode = NodeIt.Value;
			if (!SerializedNode.VoxelNode.IsA<FVoxelExecNode>())
			{
				continue;
			}

			const FVoxelGraphNodeRef NodeRef
			{
				FVoxelTerminalGraphRef(TerminalGraph),
				SerializedNode.GetNodeId(),
				SerializedNode.EdGraphNodeTitle,
				SerializedNode.EdGraphNodeName
			};

			if (SerializedNode.Errors.Num() > 0)
			{
				for (const FString& Error : SerializedNode.Errors)
				{
					VOXEL_MESSAGE(Error, "{0}: {1}", NodeRef, Error);
				}
				continue;
			}

			TSharedPtr<FVoxelExecNodeRuntimeWrapper>& Wrapper = ExecNodeKeyToWrapper_GameThread.FindOrAdd(FExecNodeKey
			{
				&TerminalGraph,
				NodeRef.NodeId
			});
			ON_SCOPE_EXIT
			{
				WrappersToKeep.Add(Wrapper);
			};

			if (Wrapper)
			{
				TVoxelArray<FName> OldPinNames;
				OldPinNames.Reserve(16);
				for (const auto& It : Wrapper->Node->GetNodeRuntime().GetNameToPinData())
				{
					OldPinNames.Add(It.Key);
				}

				TVoxelArray<FName> NewPinNames;
				NewPinNames.Reserve(16);
				for (const auto& It : SerializedNode.InputPins)
				{
					NewPinNames.Add(It.Key);
				}
				for (const auto& It : SerializedNode.OutputPins)
				{
					NewPinNames.Add(It.Key);
				}

				if (OldPinNames == NewPinNames)
				{
					continue;
				}
			}

			for (const FString& Warning : SerializedNode.Warnings)
			{
				VOXEL_MESSAGE(Warning, "{0}: {1}", NodeRef, Warning);
			}

			const TSharedRef<FVoxelExecNode> ExecNode = SerializedNode.VoxelNode.MakeSharedCopy<FVoxelExecNode>();
			ExecNode->InitializeNodeRuntime(NodeRef, false);
			ExecNode->RemoveEditorData();

			Wrapper = MakeVoxelShared<FVoxelExecNodeRuntimeWrapper>(ExecNode);
			Wrapper->Initialize(TerminalGraphInstance);
		}
	};

	const UVoxelTerminalGraph* TerminalGraph = TerminalGraphRef.GetTerminalGraph(OnChanged);
	if (!ensure(TerminalGraph))
	{
		return;
	}

	if (!TerminalGraph->IsMainTerminalGraph())
	{
		AddTerminalGraph(*TerminalGraph);
		return;
	}

#if WITH_EDITOR
	GVoxelGraphTracker->OnBaseTerminalGraphsChanged(*TerminalGraph).Add(OnChanged);
#endif

	for (const UVoxelTerminalGraph* BaseTerminalGraph : TerminalGraph->GetBaseTerminalGraphs())
	{
		AddTerminalGraph(*BaseTerminalGraph);
	}
}