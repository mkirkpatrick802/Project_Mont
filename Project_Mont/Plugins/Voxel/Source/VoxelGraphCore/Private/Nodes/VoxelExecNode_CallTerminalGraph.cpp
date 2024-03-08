// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelExecNode_CallTerminalGraph.h"
#include "Nodes/VoxelNode_Output.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphExecutor.h"
#include "VoxelGraphEvaluator.h"

TSharedRef<FVoxelInputGuidToEvaluatorRef> FVoxelExecNode_CallTerminalGraph::GetInputGuidToEvaluatorRef_GameThread() const
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	const TSharedRef<FVoxelInputGuidToEvaluatorRef> Result = MakeVoxelShared<FVoxelInputGuidToEvaluatorRef>();

	for (const auto& It : GetNodeRuntime().GetNameToPinData())
	{
		const FName PinName = It.Key;
		const FVoxelNodeRuntime::FPinData& PinData = *It.Value;

		if (!PinData.bIsInput ||
			PinName == FVoxelGraphConstants::PinName_EnableNode ||
			PinName == FVoxelGraphConstants::PinName_GraphParameter)
		{
			continue;
		}

		FGuid InputGuid;
		if (!ensure(FGuid::ParseExact(PinName.ToString(), EGuidFormats::Digits, InputGuid)))
		{
			continue;
		}

		const FVoxelGraphPinRef PinRef
		{
			GetNodeRef(),
			PinName
		};

		const TSharedRef<FVoxelGraphEvaluatorRef> EvaluatorRef = GVoxelGraphEvaluatorManager->MakeEvaluatorRef_GameThread(PinRef);
		Result->Add_CheckNew(InputGuid, EvaluatorRef);
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNode_CallTerminalGraph::FixupPinsImpl(
	const UVoxelTerminalGraph* BaseTerminalGraph,
	const FOnVoxelGraphChanged& OnChanged)
{
	VOXEL_FUNCTION_COUNTER();

	for (const auto& It : MakeCopy(GetNameToPin()))
	{
		if (It.Key == FVoxelGraphConstants::PinName_EnableNode)
		{
			continue;
		}

		RemovePin(It.Key);
	}

	AddPins();

	if (!BaseTerminalGraph)
	{
		return;
	}
	ensure(!BaseTerminalGraph->HasAnyFlags(RF_NeedLoad));

#if WITH_EDITOR
	GVoxelGraphTracker->OnInputChanged(*BaseTerminalGraph).Add(OnChanged);
	GVoxelGraphTracker->OnOutputChanged(*BaseTerminalGraph).Add(OnChanged);
#endif

	for (const FGuid& Guid : BaseTerminalGraph->GetInputs())
	{
		const FVoxelGraphInput& Input = BaseTerminalGraph->FindInputChecked(Guid);
		const FName PinName(Guid.ToString(EGuidFormats::Digits));

		// VirtualPin: inputs are virtual as they are queried by node name below
		// This avoid tricky lifetime management
		CreateInputPin(
			Input.Type,
			PinName,
			VOXEL_PIN_METADATA(void, nullptr, VirtualPin));
	}

	for (const FGuid& Guid : BaseTerminalGraph->GetOutputs())
	{
		const FVoxelGraphOutput& Output = BaseTerminalGraph->FindOutputChecked(Guid);
		const FName PinName(Guid.ToString(EGuidFormats::Digits));

		CreateOutputPin(
			Output.Type,
			PinName);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_CallTerminalGraph::Tick(FVoxelRuntime& Runtime)
{
	if (!Executor)
	{
		return;
	}

	Executor->Tick(Runtime);
}

void FVoxelExecNodeRuntime_CallTerminalGraph::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (!Executor)
	{
		return;
	}

	Executor->AddReferencedObjects(Collector);
}

FVoxelOptionalBox FVoxelExecNodeRuntime_CallTerminalGraph::GetBounds() const
{
	if (!Executor)
	{
		return {};
	}

	return Executor->GetBounds();
}