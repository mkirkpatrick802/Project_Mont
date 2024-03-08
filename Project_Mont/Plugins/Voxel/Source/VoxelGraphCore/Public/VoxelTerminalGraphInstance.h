// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"
#include "VoxelGraphNodeRef.h"
#include "VoxelParameterPath.h"

class FVoxelRuntime;
class FVoxelCallstack;
class FVoxelRuntimeInfo;
class FVoxelGraphEvaluatorRef;
class FVoxelTerminalGraphInstance;
class FVoxelExecNodeRuntimeWrapper;
class FVoxelParameterOverridesRuntime;
struct FVoxelExecNode;
struct FVoxelGraphEvaluatorRefWrapper;

extern VOXELGRAPHCORE_API int32 GVoxelMaxRecursionDepth;

using FVoxelInputGuidToEvaluatorRef = TVoxelMap<FGuid, TSharedPtr<const FVoxelGraphEvaluatorRef>>;

struct FVoxelInputComputeInfo
{
	// The terminal graph instance the inputs are in
	// This is a parent instance, but might not be the immediate parent in case we are a recursive CallGraphParameter
	TWeakPtr<FVoxelTerminalGraphInstance> WeakTerminalGraphInstance;
	// GUID to evaluator ref in the terminal graph calling us
	TSharedPtr<const FVoxelInputGuidToEvaluatorRef> InputGuidToEvaluatorRef;

	FVoxelInputComputeInfo() = default;
	FVoxelInputComputeInfo(
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		const TSharedRef<const FVoxelInputGuidToEvaluatorRef>& InputGuidToEvaluatorRef)
		: WeakTerminalGraphInstance(TerminalGraphInstance)
		, InputGuidToEvaluatorRef(InputGuidToEvaluatorRef)
	{
	}

	FORCEINLINE bool operator==(const FVoxelInputComputeInfo& Other) const
	{
		return
			WeakTerminalGraphInstance == Other.WeakTerminalGraphInstance &&
			InputGuidToEvaluatorRef == Other.InputGuidToEvaluatorRef;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelInputComputeInfo& Info)
	{
		return
			GetTypeHash(Info.WeakTerminalGraphInstance) ^
			GetTypeHash(Info.InputGuidToEvaluatorRef);
	}
};

struct FVoxelTerminalGraphInstanceChildKey
{
public:
	// Main graph, used to resolve function calls
	TWeakObjectPtr<const UVoxelGraph> Graph;
	// The child graph
	FVoxelTerminalGraphRef TerminalGraphRef;
	// The node in the current graph calling this child graph
	FVoxelGraphNodeRef NodeRef;

public:
	FVoxelParameterPath ParameterPath;
	TSharedPtr<const FVoxelParameterOverridesRuntime> ParameterOverridesRuntime;
	FVoxelInputComputeInfo GraphInputComputeInfo;
	FVoxelInputComputeInfo FunctionInputComputeInfo;

	FVoxelTerminalGraphInstanceChildKey() = default;

	FVoxelTerminalGraphInstanceChildKey(
		const TWeakObjectPtr<const UVoxelGraph>& Graph,
		const FVoxelTerminalGraphRef& TerminalGraphRef,
		const FVoxelGraphNodeRef& NodeRef,
		const FVoxelParameterPath& ParameterPath,
		const TSharedPtr<const FVoxelParameterOverridesRuntime>& ParameterOverridesRuntime,
		const FVoxelInputComputeInfo& GraphInputComputeInfo,
		const FVoxelInputComputeInfo& FunctionInputComputeInfo)
		: Graph(Graph)
		, TerminalGraphRef(TerminalGraphRef)
		, NodeRef(NodeRef)
		, ParameterPath(ParameterPath)
		, ParameterOverridesRuntime(ParameterOverridesRuntime)
		, GraphInputComputeInfo(GraphInputComputeInfo)
		, FunctionInputComputeInfo(FunctionInputComputeInfo)
	{
	}

	FORCEINLINE bool operator==(const FVoxelTerminalGraphInstanceChildKey& Other) const
	{
		return
			Graph == Other.Graph &&
			TerminalGraphRef == Other.TerminalGraphRef &&
			NodeRef == Other.NodeRef &&
			ParameterPath == Other.ParameterPath &&
			ParameterOverridesRuntime == Other.ParameterOverridesRuntime &&
			GraphInputComputeInfo == Other.GraphInputComputeInfo &&
			FunctionInputComputeInfo == Other.FunctionInputComputeInfo;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelTerminalGraphInstanceChildKey& Key)
	{
		return
			GetTypeHash(Key.Graph) ^
			GetTypeHash(Key.TerminalGraphRef) ^
			GetTypeHash(Key.NodeRef) ^
			GetTypeHash(Key.ParameterPath) ^
			GetTypeHash(Key.ParameterOverridesRuntime) ^
			GetTypeHash(Key.GraphInputComputeInfo) ^
			GetTypeHash(Key.FunctionInputComputeInfo);
	}
};

class VOXELGRAPHCORE_API FVoxelTerminalGraphInstance : public TSharedFromThis<FVoxelTerminalGraphInstance>
{
public:
	// Main graph, used to resolve function calls
	const TWeakObjectPtr<const UVoxelGraph> Graph;
	// Terminal graph we are instancing
	const FVoxelTerminalGraphRef TerminalGraphRef;
	// Callstack that produced this instance,
	// ie FunctionCallA.FunctionCallB.FunctionCallC
	const TSharedRef<const FVoxelCallstack> Callstack;
	// Info about the runtime owning this instance
	// Only case where this might be different within a single query is if we're querying a channel,
	// as each brush will have its own runtime
	const TSharedRef<const FVoxelRuntimeInfo> RuntimeInfo;
	// Parent instance for debugging
	const TWeakPtr<const FVoxelTerminalGraphInstance> WeakParentInstance;

public:
	// Parameter path, will be used as prefix when accessing parameters
	// Set by CallGraphParameter
	const FVoxelParameterPath ParameterPath;
	// Parameter overrides, set by CallGraph (but not CallGraphParameter)
	const TSharedPtr<const FVoxelParameterOverridesRuntime> ParameterOverridesRuntime;
	// Graph inputs
	const FVoxelInputComputeInfo GraphInputComputeInfo;
	// Function inputs
	const FVoxelInputComputeInfo FunctionInputComputeInfo;

public:
	VOXEL_COUNT_INSTANCES();

	static TSharedRef<FVoxelTerminalGraphInstance> Make(
		const UVoxelGraph& Graph,
		const FVoxelTerminalGraphRef& TerminalGraphRef,
		const TSharedRef<const FVoxelRuntimeInfo>& RuntimeInfo,
		const TSharedRef<const FVoxelParameterOverridesRuntime>& ParameterOverridesRuntime);

	// Creates a dummy instance to have a different QueryRuntimeInfo
	static TSharedRef<FVoxelTerminalGraphInstance> MakeDummy(const TSharedRef<const FVoxelRuntimeInfo>& RuntimeInfo);

public:
	int32 GetDepth() const;
	TSharedRef<FVoxelTerminalGraphInstance> GetChild(const FVoxelTerminalGraphInstanceChildKey& Key);
	TVoxelFutureValue<FVoxelGraphEvaluatorRefWrapper> CreateOutputEvaluatorRef(const FGuid& OutputGuid);

private:
	mutable FVoxelCriticalSection CriticalSection;
	// Keep all children alive
	TVoxelMap<FVoxelTerminalGraphInstanceChildKey, TSharedPtr<FVoxelTerminalGraphInstance>> ChildKeyToChild_RequiresLock;
	TVoxelMap<FGuid, TVoxelFutureValue<FVoxelGraphEvaluatorRefWrapper>> OutputGuidToFutureEvaluatorRef_RequiresLock;

	FVoxelTerminalGraphInstance(
		const TWeakObjectPtr<const UVoxelGraph>& Graph,
		const FVoxelTerminalGraphRef& TerminalGraphRef,
		const TSharedRef<const FVoxelCallstack>& Callstack,
		const TSharedRef<const FVoxelRuntimeInfo>& RuntimeInfo,
		const TSharedPtr<const FVoxelTerminalGraphInstance>& ParentInstance,
		const FVoxelParameterPath& ParameterPath,
		const TSharedPtr<const FVoxelParameterOverridesRuntime>& ParameterOverridesRuntime,
		const FVoxelInputComputeInfo& GraphInputComputeInfo,
		const FVoxelInputComputeInfo& FunctionInputComputeInfo);
};