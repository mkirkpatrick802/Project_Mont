// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTerminalGraphInstance.h"
#include "VoxelGraph.h"
#include "VoxelCallstack.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelParameterOverridesRuntime.h"
#include "Nodes/VoxelNode_Output.h"

VOXEL_CONSOLE_VARIABLE(
	VOXELGRAPHCORE_API, int32, GVoxelMaxRecursionDepth, 16,
	"voxel.MaxRecursionDepth",
	"");

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelTerminalGraphInstance);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<FVoxelTerminalGraphInstance> FVoxelTerminalGraphInstance::Make(
	const UVoxelGraph& Graph,
	const FVoxelTerminalGraphRef& TerminalGraphRef,
	const TSharedRef<const FVoxelRuntimeInfo>& RuntimeInfo,
	const TSharedRef<const FVoxelParameterOverridesRuntime>& ParameterOverridesRuntime)
{
	return MakeVoxelShareable(new (GVoxelMemory) FVoxelTerminalGraphInstance(
		&Graph,
		TerminalGraphRef,
		MakeShared<FVoxelCallstack>(),
		RuntimeInfo,
		nullptr,
		FVoxelParameterPath(),
		ParameterOverridesRuntime,
		{},
		{}));
}

TSharedRef<FVoxelTerminalGraphInstance> FVoxelTerminalGraphInstance::MakeDummy(const TSharedRef<const FVoxelRuntimeInfo>& RuntimeInfo)
{
	return MakeVoxelShareable(new (GVoxelMemory) FVoxelTerminalGraphInstance(
		{},
		FVoxelTerminalGraphRef(),
		MakeShared<FVoxelCallstack>(),
		RuntimeInfo,
		nullptr,
		FVoxelParameterPath(),
		nullptr,
		{},
		{}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int32 FVoxelTerminalGraphInstance::GetDepth() const
{
	int32 Depth = 0;
	for (const FVoxelCallstack* It = &Callstack.Get(); It; It = It->GetParent())
	{
		Depth++;
	}
	return Depth;
}

TSharedRef<FVoxelTerminalGraphInstance> FVoxelTerminalGraphInstance::GetChild(const FVoxelTerminalGraphInstanceChildKey& Key)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(CriticalSection);
	ensure(GetDepth() < GVoxelMaxRecursionDepth);

	TSharedPtr<FVoxelTerminalGraphInstance>& Instance = ChildKeyToChild_RequiresLock.FindOrAdd(Key);
	if (!Instance)
	{
		Instance = MakeVoxelShareable(new (GVoxelMemory) FVoxelTerminalGraphInstance(
			Key.Graph,
			Key.TerminalGraphRef,
			Callstack->MakeChild({}, Key.NodeRef, RuntimeInfo->GetRuntimeProvider().GetWeakObjectPtr()),
			RuntimeInfo,
			AsShared(),
			Key.ParameterPath,
			Key.ParameterOverridesRuntime,
			Key.GraphInputComputeInfo,
			Key.FunctionInputComputeInfo));
	}

	ensure(Instance->TerminalGraphRef == Key.TerminalGraphRef);
	ensure(Instance->Callstack->GetNodeRef() == Key.NodeRef);
	ensure(Instance->ParameterPath == Key.ParameterPath);
	ensure(Instance->ParameterOverridesRuntime == Key.ParameterOverridesRuntime);
	ensure(Instance->GraphInputComputeInfo == Key.GraphInputComputeInfo);
	ensure(Instance->FunctionInputComputeInfo == Key.FunctionInputComputeInfo);

	return Instance.ToSharedRef();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelFutureValue<FVoxelGraphEvaluatorRefWrapper> FVoxelTerminalGraphInstance::CreateOutputEvaluatorRef(const FGuid& OutputGuid)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	TVoxelFutureValue<FVoxelGraphEvaluatorRefWrapper>& FutureEvaluatorRef = OutputGuidToFutureEvaluatorRef_RequiresLock.FindOrAdd(OutputGuid);

	if (FutureEvaluatorRef.IsValid())
	{
		return FutureEvaluatorRef;
	}

	VOXEL_FUNCTION_COUNTER();

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

	FutureEvaluatorRef = GVoxelGraphEvaluatorManager->MakeEvaluatorRef_AnyThread(PinRef);
	return FutureEvaluatorRef;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTerminalGraphInstance::FVoxelTerminalGraphInstance(
	const TWeakObjectPtr<const UVoxelGraph>& Graph,
	const FVoxelTerminalGraphRef& TerminalGraphRef,
	const TSharedRef<const FVoxelCallstack>& Callstack,
	const TSharedRef<const FVoxelRuntimeInfo>& RuntimeInfo,
	const TSharedPtr<const FVoxelTerminalGraphInstance>& ParentInstance,
	const FVoxelParameterPath& ParameterPath,
	const TSharedPtr<const FVoxelParameterOverridesRuntime>& ParameterOverridesRuntime,
	const FVoxelInputComputeInfo& GraphInputComputeInfo,
	const FVoxelInputComputeInfo& FunctionInputComputeInfo)
	: Graph(Graph)
	, TerminalGraphRef(TerminalGraphRef)
	, Callstack(Callstack)
	, RuntimeInfo(RuntimeInfo)
	, WeakParentInstance(ParentInstance)
	, ParameterPath(ParameterPath)
	, ParameterOverridesRuntime(ParameterOverridesRuntime)
	, GraphInputComputeInfo(GraphInputComputeInfo)
	, FunctionInputComputeInfo(FunctionInputComputeInfo)
{
}