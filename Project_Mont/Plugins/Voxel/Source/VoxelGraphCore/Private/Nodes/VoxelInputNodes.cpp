// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelInputNodes.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelTerminalGraphInstance.h"

void FVoxelNode_Input_WithoutDefaultPin::PreCompile()
{
	ensure(DefaultValue.IsValid());

	RuntimeDefaultValue = FVoxelPinType::MakeRuntimeValue(
		GetPin(ValuePin).GetType(),
		DefaultValue);

	ensure(RuntimeDefaultValue.IsValid());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_Input_WithoutDefaultPin, Value)
{
	const FVoxelInputComputeInfo& InputComputeInfo = bIsGraphInput
		? Query.GetTerminalGraphInstance().GraphInputComputeInfo
		: Query.GetTerminalGraphInstance().FunctionInputComputeInfo;

	const TSharedPtr<const FVoxelInputGuidToEvaluatorRef>& InputGuidToEvaluatorRef = InputComputeInfo.InputGuidToEvaluatorRef;
	if (!InputGuidToEvaluatorRef)
	{
		// If we don't have input values use our default value
		return RuntimeDefaultValue;
	}

	const TSharedPtr<const FVoxelGraphEvaluatorRef> EvaluatorRef = InputGuidToEvaluatorRef->FindRef(Guid);
	if (!EvaluatorRef)
	{
		// If we can't find a matching input this is likely a graph override
		return RuntimeDefaultValue;
	}

	const TSharedPtr<FVoxelTerminalGraphInstance> TerminalGraphInstance = InputComputeInfo.WeakTerminalGraphInstance.Pin();
	if (!ensureVoxelSlow(TerminalGraphInstance))
	{
		return {};
	}

	return EvaluatorRef->Compute(Query.MakeNewQuery(TerminalGraphInstance.ToSharedRef()));
}

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_Input_WithDefaultPin, Value)
{
	const FVoxelInputComputeInfo& InputComputeInfo = bIsGraphInput
		? Query.GetTerminalGraphInstance().GraphInputComputeInfo
		: Query.GetTerminalGraphInstance().FunctionInputComputeInfo;

	const TSharedPtr<const FVoxelInputGuidToEvaluatorRef>& InputGuidToEvaluatorRef = InputComputeInfo.InputGuidToEvaluatorRef;
	if (!InputGuidToEvaluatorRef)
	{
		// If we don't have input values use our input pin
		return Get(DefaultPin, Query);
	}

	const TSharedPtr<const FVoxelGraphEvaluatorRef> EvaluatorRef = InputGuidToEvaluatorRef->FindRef(Guid);
	if (!EvaluatorRef)
	{
		// If we can't find a matching input this is likely a graph override
		return Get(DefaultPin, Query);
	}

	const TSharedPtr<FVoxelTerminalGraphInstance> TerminalGraphInstance = InputComputeInfo.WeakTerminalGraphInstance.Pin();
	if (!ensure(TerminalGraphInstance))
	{
		return {};
	}

	const TSharedPtr<const FVoxelGraphEvaluator> Evaluator = EvaluatorRef->GetEvaluator(Query);
	if (!Evaluator)
	{
		// Failed to compile
		return {};
	}

	if (Evaluator->IsDefaultValue())
	{
		// Use default
		return Get(DefaultPin, Query);
	}

	FVoxelTaskReferencer::Get().AddEvaluator(Evaluator.ToSharedRef());

	return Evaluator->Get(Query.MakeNewQuery(TerminalGraphInstance.ToSharedRef()));
}