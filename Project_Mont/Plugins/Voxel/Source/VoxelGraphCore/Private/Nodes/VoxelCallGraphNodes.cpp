// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelCallGraphNodes.h"
#include "VoxelGraph.h"
#include "VoxelInlineGraph.h"
#include "VoxelGraphExecutor.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelTerminalGraphInstance.h"
#include "VoxelParameterOverridesRuntime.h"
#include "Nodes/VoxelNode_Output.h"

void FVoxelExecNode_CallGraph::FixupPins(
	const UVoxelGraph& Context,
	const FOnVoxelGraphChanged& OnChanged)
{
	if (!Graph)
	{
		FixupPinsImpl(nullptr, OnChanged);
		return;
	}

	FixupPinsImpl(&Graph->GetMainTerminalGraph(), OnChanged);
}

FVoxelComputeValue FVoxelExecNode_CallGraph::CompileCompute(const FName PinName) const
{
	VOXEL_FUNCTION_COUNTER();

	FGuid OutputGuid;
	if (!ensure(FGuid::ParseExact(PinName.ToString(), EGuidFormats::Digits, OutputGuid)))
	{
		return nullptr;
	}

	const FVoxelGraphNodeRef NodeRef
	{
		FVoxelTerminalGraphRef(Graph->GetMainTerminalGraph()),
		FVoxelGraphConstants::GetOutputNodeId(OutputGuid),
		FName("Output " + OutputGuid.ToString()),
		{},
	};
	const FVoxelGraphPinRef PinRef
	{
		NodeRef,
		VOXEL_PIN_NAME(FVoxelNode_Output, ValuePin)
	};

	const FVoxelNodeRuntime::FPinData& PinData = GetNodeRuntime().GetPinData(PinName);

	return [
		this,
		PinType = PinData.Type,
		EvaluatorRef = GVoxelGraphEvaluatorManager->MakeEvaluatorRef_GameThread(PinRef),
		TerminalGraphRef = FVoxelTerminalGraphRef(Graph->GetMainTerminalGraph()),
		ParameterOverridesRuntime = FVoxelParameterOverridesRuntime::Create(Graph.Get()),
		InputGuidToEvaluatorRef = GetInputGuidToEvaluatorRef_GameThread()](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));

		if (Query.GetTerminalGraphInstance().GetDepth() >= GVoxelMaxRecursionDepth)
		{
			VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
			return {};
		}

		// CallGraph: both graph and function compute info are the same
		const FVoxelInputComputeInfo InputComputeInfo
		{
			Query.GetSharedTerminalGraphInstance(),
			InputGuidToEvaluatorRef
		};

		const FVoxelTerminalGraphInstanceChildKey Key
		{
			TerminalGraphRef.Graph,
			TerminalGraphRef,
			GetNodeRef(),
			FVoxelParameterPath::MakeEmpty(),
			ParameterOverridesRuntime,
			InputComputeInfo,
			InputComputeInfo
		};
		const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = Query.GetTerminalGraphInstance().GetChild(Key);

		// Wrap in a task to check type
		return
			MakeVoxelTask()
			.Execute(PinType, [=]
			{
				return EvaluatorRef->Compute(Query.MakeNewQuery(TerminalGraphInstance));
			});
	};
}

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_CallGraph::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_CallGraph>(SharedThis);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_CallGraph::Create()
{
	UVoxelGraph* Graph = Node.Graph.Get();
	if (!Graph)
	{
		return;
	}

	if (GetTerminalGraphInstance()->GetDepth() >= GVoxelMaxRecursionDepth)
	{
		VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
		return;
	}

	const FVoxelTerminalGraphRef TerminalGraphRef = FVoxelTerminalGraphRef(Graph->GetMainTerminalGraph());

	// CallGraph: both graph and function compute info are the same
	const FVoxelInputComputeInfo InputComputeInfo
	{
		GetTerminalGraphInstance(),
		Node.GetInputGuidToEvaluatorRef_GameThread()
	};

	const FVoxelTerminalGraphInstanceChildKey Key
	{
		TerminalGraphRef.Graph,
		TerminalGraphRef,
		GetNodeRef(),
		FVoxelParameterPath::MakeEmpty(),
		FVoxelParameterOverridesRuntime::Create(Graph),
		InputComputeInfo,
		InputComputeInfo
	};
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = GetTerminalGraphInstance()->GetChild(Key);

	Executor = FVoxelGraphExecutor::Create_GameThread(TerminalGraphRef, TerminalGraphInstance);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNode_CallGraphParameter::AddPins()
{
	CreateInputPin(
		FVoxelPinType::Make<FVoxelInlineGraph>(),
		FVoxelGraphConstants::PinName_GraphParameter,
		VOXEL_PIN_METADATA(FVoxelInlineGraph, nullptr, ConstantPin));
}

void FVoxelExecNode_CallGraphParameter::FixupPins(
	const UVoxelGraph& Context,
	const FOnVoxelGraphChanged& OnChanged)
{
	if (!BaseGraph)
	{
		FixupPinsImpl(nullptr, OnChanged);
		return;
	}

	FixupPinsImpl(&BaseGraph->GetMainTerminalGraph(), OnChanged);
}

FVoxelComputeValue FVoxelExecNode_CallGraphParameter::CompileCompute(const FName PinName) const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelNodeRuntime::FPinData& PinData = GetNodeRuntime().GetPinData(PinName);

	FGuid OutputGuid;
	if (!ensure(FGuid::ParseExact(PinName.ToString(), EGuidFormats::Digits, OutputGuid)))
	{
		return nullptr;
	}

	return [
		this,
		OutputGuid,
		PinType = PinData.Type,
		InputGuidToEvaluatorRef = GetInputGuidToEvaluatorRef_GameThread()](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));

		if (Query.GetTerminalGraphInstance().GetDepth() >= GVoxelMaxRecursionDepth)
		{
			VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
			return FVoxelRuntimePinValue(PinType);
		}

		const TValue<FVoxelInlineGraph> FutureInlineGraph = GetNodeRuntime().Get<FVoxelInlineGraph>(
			FVoxelPinRef(FVoxelGraphConstants::PinName_GraphParameter),
			Query);

		return
			MakeVoxelTask()
			.Dependency(FutureInlineGraph)
			.Execute(PinType, [=]() -> FVoxelFutureValue
			{
				const FVoxelInlineGraph& InlineGraph = FutureInlineGraph.Get_CheckCompleted();
				if (!InlineGraph.ParameterPath.IsSet())
				{
					VOXEL_MESSAGE(Error, "{0}: Invalid graph parameter", this);
					return {};
				}
				ensure(!InlineGraph.Graph.IsExplicitlyNull());

				// CallGraph: both graph and function compute info are the same
				const FVoxelInputComputeInfo InputComputeInfo
				{
					Query.GetSharedTerminalGraphInstance(),
					InputGuidToEvaluatorRef
				};

				const FVoxelTerminalGraphInstanceChildKey Key
				{
					InlineGraph.Graph,
					FVoxelTerminalGraphRef(InlineGraph.Graph, GVoxelMainTerminalGraphGuid),
					GetNodeRef(),
					InlineGraph.ParameterPath.GetValue(),
					Query.GetTerminalGraphInstance().ParameterOverridesRuntime,
					InputComputeInfo,
					InputComputeInfo
				};
				const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = Query.GetTerminalGraphInstance().GetChild(Key);
				const TValue<FVoxelGraphEvaluatorRefWrapper> FutureEvaluatorRef = TerminalGraphInstance->CreateOutputEvaluatorRef(OutputGuid);

				return
					MakeVoxelTask()
					.Dependency(FutureEvaluatorRef)
					.Execute(PinType, [=]() -> FVoxelFutureValue
					{
						const TSharedPtr<const FVoxelGraphEvaluatorRef> EvaluatorRef = FutureEvaluatorRef.Get_CheckCompleted().EvaluatorRef;
						if (!EvaluatorRef)
						{
							VOXEL_MESSAGE(Error, "{0}: Invalid reference", this);
							return {};
						}
						return EvaluatorRef->Compute(Query.MakeNewQuery(TerminalGraphInstance));
					});
			});
	};
}

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_CallGraphParameter::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_CallGraphParameter>(SharedThis);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_CallGraphParameter::Create()
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelInlineGraph& InlineGraph = GetConstantPin(FVoxelPinRef(FVoxelGraphConstants::PinName_GraphParameter)).Get<FVoxelInlineGraph>();
	UVoxelGraph* Graph = InlineGraph.Graph.Get();
	if (!Graph ||
		!ensure(InlineGraph.ParameterPath.IsSet()))
	{
		return;
	}

	if (GetTerminalGraphInstance()->GetDepth() >= GVoxelMaxRecursionDepth)
	{
		VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
		return;
	}

	const FVoxelTerminalGraphRef TerminalGraphRef(Graph->GetMainTerminalGraph());

	// CallGraph: both graph and function compute info are the same
	const FVoxelInputComputeInfo InputComputeInfo
	{
		GetTerminalGraphInstance(),
		Node.GetInputGuidToEvaluatorRef_GameThread()
	};

	const FVoxelTerminalGraphInstanceChildKey Key
	{
		TerminalGraphRef.Graph,
		TerminalGraphRef,
		GetNodeRef(),
		InlineGraph.ParameterPath.GetValue(),
		GetTerminalGraphInstance()->ParameterOverridesRuntime,
		InputComputeInfo,
		InputComputeInfo
	};
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = GetTerminalGraphInstance()->GetChild(Key);

	Executor = FVoxelGraphExecutor::Create_GameThread(TerminalGraphRef, TerminalGraphInstance);
}