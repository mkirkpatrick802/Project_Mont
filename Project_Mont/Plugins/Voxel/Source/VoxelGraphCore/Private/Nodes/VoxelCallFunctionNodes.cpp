// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelCallFunctionNodes.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphExecutor.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelFunctionLibraryAsset.h"
#include "VoxelTerminalGraphInstance.h"
#include "Nodes/VoxelNode_Output.h"

void FVoxelExecNode_CallFunction::FixupPins(
	const UVoxelGraph& Context,
	const FOnVoxelGraphChanged& OnChanged)
{
	if (ContextOverride &&
		ContextOverride != &Context)
	{
		FixupPins(*ContextOverride, OnChanged);
		return;
	}

#if WITH_EDITOR
	GVoxelGraphTracker->OnTerminalGraphChanged(Context).Add(OnChanged);
#endif

	const UVoxelTerminalGraph* TerminalGraph = Context.FindTerminalGraph(Guid);
	if (!TerminalGraph)
	{
		FixupPinsImpl(nullptr, OnChanged);
		return;
	}

	FixupPinsImpl(TerminalGraph, OnChanged);
}

FVoxelComputeValue FVoxelExecNode_CallFunction::CompileCompute(const FName PinName) const
{
	VOXEL_FUNCTION_COUNTER();

	FGuid OutputGuid;
	if (!ensure(FGuid::ParseExact(PinName.ToString(), EGuidFormats::Digits, OutputGuid)))
	{
		return nullptr;
	}

	const FVoxelNodeRuntime::FPinData& PinData = GetNodeRuntime().GetPinData(PinName);

	if (ContextOverride)
	{
		if (!ContextOverride->FindTerminalGraph(Guid))
		{
			VOXEL_MESSAGE(Error, "{0}: Invalid Call Parent");
			return nullptr;
		}

		const FVoxelTerminalGraphRef TerminalGraphRef(ContextOverride, Guid);
		const FVoxelGraphNodeRef NodeRef
		{
			TerminalGraphRef,
			FVoxelGraphConstants::GetOutputNodeId(OutputGuid),
			FName("Output " + OutputGuid.ToString()),
			{},
		};
		const FVoxelGraphPinRef PinRef
		{
			NodeRef,
			VOXEL_PIN_NAME(FVoxelNode_Output, ValuePin)
		};

		return [
			this,
			PinType = PinData.Type,
			EvaluatorRef = GVoxelGraphEvaluatorManager->MakeEvaluatorRef_GameThread(PinRef),
			TerminalGraphRef,
			InputGuidToEvaluatorRef = GetInputGuidToEvaluatorRef_GameThread()](const FVoxelQuery& Query) -> FVoxelFutureValue
		{
			checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));

			if (Query.GetTerminalGraphInstance().GetDepth() >= GVoxelMaxRecursionDepth)
			{
				VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
				return {};
			}

			// CallFunction: override function inputs, but keep graph inputs
			const FVoxelInputComputeInfo FunctionInputComputeInfo
			{
				Query.GetSharedTerminalGraphInstance(),
				InputGuidToEvaluatorRef
			};

			const FVoxelTerminalGraphInstanceChildKey Key
			{
				TerminalGraphRef.Graph,
				TerminalGraphRef,
				GetNodeRef(),
				Query.GetTerminalGraphInstance().ParameterPath,
				Query.GetTerminalGraphInstance().ParameterOverridesRuntime,
				Query.GetTerminalGraphInstance().GraphInputComputeInfo,
				FunctionInputComputeInfo
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

	return [
		this,
		Guid = Guid,
		PinType = PinData.Type,
		OutputGuid,
		InputGuidToEvaluatorRef = GetInputGuidToEvaluatorRef_GameThread()](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));

		if (Query.GetTerminalGraphInstance().GetDepth() >= GVoxelMaxRecursionDepth)
		{
			VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
			return {};
		}

		FVoxelTerminalGraphRef TerminalGraphRef;
		TerminalGraphRef.Graph = Query.GetTerminalGraphInstance().Graph;
		TerminalGraphRef.TerminalGraphGuid = Guid;

		// CallFunction: override function inputs, but keep graph inputs
		const FVoxelInputComputeInfo FunctionInputComputeInfo
		{
			Query.GetSharedTerminalGraphInstance(),
			InputGuidToEvaluatorRef
		};

		const FVoxelTerminalGraphInstanceChildKey Key
		{
			Query.GetTerminalGraphInstance().Graph,
			TerminalGraphRef,
			GetNodeRef(),
			Query.GetTerminalGraphInstance().ParameterPath,
			Query.GetTerminalGraphInstance().ParameterOverridesRuntime,
			Query.GetTerminalGraphInstance().GraphInputComputeInfo,
			FunctionInputComputeInfo
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
	};
}

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_CallFunction::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_CallFunction>(SharedThis);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_CallFunction::Create()
{
	VOXEL_FUNCTION_COUNTER();

	const UVoxelGraph* Graph = GetTerminalGraphInstance()->Graph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	if (!Node.Guid.IsValid() ||
		!Graph->FindTerminalGraph(Node.Guid))
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid reference", this);
		return;
	}

	if (GetTerminalGraphInstance()->GetDepth() >= GVoxelMaxRecursionDepth)
	{
		VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
		return;
	}

	FVoxelTerminalGraphRef TerminalGraphRef;
	if (Node.ContextOverride)
	{
		TerminalGraphRef.Graph = Node.ContextOverride;
	}
	else
	{
		TerminalGraphRef.Graph = GetTerminalGraphInstance()->Graph;
	}
	TerminalGraphRef.TerminalGraphGuid = Node.Guid;

	// CallFunction: override function inputs, but keep graph inputs
	const FVoxelInputComputeInfo FunctionInputComputeInfo
	{
		GetTerminalGraphInstance(),
		Node.GetInputGuidToEvaluatorRef_GameThread()
	};

	const FVoxelTerminalGraphInstanceChildKey Key
	{
		TerminalGraphRef.Graph,
		TerminalGraphRef,
		GetNodeRef(),
		GetTerminalGraphInstance()->ParameterPath,
		GetTerminalGraphInstance()->ParameterOverridesRuntime,
		GetTerminalGraphInstance()->GraphInputComputeInfo,
		FunctionInputComputeInfo
	};
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = GetTerminalGraphInstance()->GetChild(Key);

	Executor = FVoxelGraphExecutor::Create_GameThread(
		FVoxelTerminalGraphRef(Graph, Node.Guid),
		TerminalGraphInstance);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNode_CallExternalFunction::FixupPins(
	const UVoxelGraph& Context,
	const FOnVoxelGraphChanged& OnChanged)
{
	if (!FunctionLibrary)
	{
		FixupPinsImpl(nullptr, OnChanged);
		return;
	}

	const UVoxelTerminalGraph* TerminalGraph = FunctionLibrary->GetGraph().FindTerminalGraph(Guid);
	if (!TerminalGraph)
	{
		FixupPinsImpl(nullptr, OnChanged);
		return;
	}

	FixupPinsImpl(TerminalGraph, OnChanged);
}

FVoxelComputeValue FVoxelExecNode_CallExternalFunction::CompileCompute(const FName PinName) const
{
	VOXEL_FUNCTION_COUNTER();

	if (!FunctionLibrary)
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid reference (FunctionLibrary is null)", this);
		return nullptr;
	}

	const UVoxelTerminalGraph* TerminalGraph = FunctionLibrary->GetGraph().FindTerminalGraph(Guid);
	if (!TerminalGraph)
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid reference (TerminalGraph is null)", this);
		return nullptr;
	}

	FGuid OutputGuid;
	if (!ensure(FGuid::ParseExact(PinName.ToString(), EGuidFormats::Digits, OutputGuid)))
	{
		return nullptr;
	}

	const FVoxelGraphNodeRef NodeRef
	{
		FVoxelTerminalGraphRef(TerminalGraph),
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
		TerminalGraphRef = FVoxelTerminalGraphRef(TerminalGraph),
		InputGuidToEvaluatorRef = GetInputGuidToEvaluatorRef_GameThread()](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));

		if (Query.GetTerminalGraphInstance().GetDepth() >= GVoxelMaxRecursionDepth)
		{
			VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
			return {};
		}

		// CallExternalFunction has no graph and thus no graph inputs
		const FVoxelInputComputeInfo FunctionInputComputeInfo
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
			nullptr,
			{},
			FunctionInputComputeInfo
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

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_CallExternalFunction::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_CallExternalFunction>(SharedThis);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_CallExternalFunction::Create()
{
	VOXEL_FUNCTION_COUNTER();

	if (!Node.FunctionLibrary)
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid reference (FunctionLibrary is null)", this);
		return;
	}

	const UVoxelTerminalGraph* TerminalGraph = Node.FunctionLibrary->GetGraph().FindTerminalGraph(Node.Guid);
	if (!TerminalGraph)
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid reference (TerminalGraph is null)", this);
		return;
	}

	if (GetTerminalGraphInstance()->GetDepth() >= GVoxelMaxRecursionDepth)
	{
		VOXEL_MESSAGE(Error, "{0}: Max recursion depth reached", this);
		return;
	}

	const FVoxelTerminalGraphRef TerminalGraphRef(TerminalGraph);

	// CallExternalFunction has no graph and thus no graph inputs
	const FVoxelInputComputeInfo FunctionInputComputeInfo
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
		nullptr,
		{},
		FunctionInputComputeInfo
	};
	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = GetTerminalGraphInstance()->GetChild(Key);

	Executor = FVoxelGraphExecutor::Create_GameThread(
		FVoxelTerminalGraphRef(TerminalGraph),
		TerminalGraphInstance);
}