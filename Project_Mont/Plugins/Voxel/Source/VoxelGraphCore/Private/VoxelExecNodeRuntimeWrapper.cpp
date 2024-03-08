// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelExecNodeRuntimeWrapper.h"
#include "VoxelBuffer.h"

void FVoxelExecNodeRuntimeWrapper::Initialize(const TSharedRef<FVoxelTerminalGraphInstance>& NewTerminalGraphInstance)
{
	TerminalGraphInstance = NewTerminalGraphInstance;

	EnableNodeValue =
		Node->GetNodeRuntime().MakeDynamicValueFactory(Node->EnableNodePin)
		.AddRef(Node)
		.RunSynchronously()
		.Compute(TerminalGraphInstance.ToSharedRef());

	EnableNodeValue.OnChanged_GameThread(MakeWeakPtrLambda(this, [=](const bool bNewEnableNode)
	{
		if (!bNewEnableNode)
		{
			NodeRuntime_GameThread.Reset();
			ConstantPins_GameThread = {};
			return;
		}

		if (NodeRuntime_GameThread)
		{
			return;
		}

		if (ConstantPins_GameThread.Num() == 0)
		{
			ComputeConstantPins();
		}

		// NodeRuntime_GameThread will be set if ComputeConstantPins completes right away
		if (!NodeRuntime_GameThread)
		{
			OnConstantValueUpdated();
		}
	}));
}

void FVoxelExecNodeRuntimeWrapper::Tick(FVoxelRuntime& Runtime) const
{
	ensure(IsInGameThread());

	if (!NodeRuntime_GameThread)
	{
		return;
	}

	FVoxelQueryScope Scope(nullptr, TerminalGraphInstance.Get());
	NodeRuntime_GameThread->Tick(Runtime);
}

void FVoxelExecNodeRuntimeWrapper::AddReferencedObjects(FReferenceCollector& Collector) const
{
	if (!NodeRuntime_GameThread)
	{
		return;
	}

	FVoxelQueryScope Scope(nullptr, TerminalGraphInstance.Get());
	NodeRuntime_GameThread->AddReferencedObjects(Collector);
}

FVoxelOptionalBox FVoxelExecNodeRuntimeWrapper::GetBounds() const
{
	ensure(IsInGameThread());

	if (!NodeRuntime_GameThread)
	{
		return {};
	}

	FVoxelQueryScope Scope(nullptr, TerminalGraphInstance.Get());
	return NodeRuntime_GameThread->GetBounds();
}

void FVoxelExecNodeRuntimeWrapper::ComputeConstantPins()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());
	ensure(ConstantPins_GameThread.Num() == 0);

	// Pre-allocate ConstantPins_GameThread to avoid OnChanged calling OnConstantValueUpdated right away
	for (const auto& It : Node->GetNodeRuntime().GetNameToPinData())
	{
		const FVoxelNodeRuntime::FPinData& PinData = *It.Value;
		if (!PinData.Metadata.bConstantPin ||
			!ensure(PinData.bIsInput))
		{
			continue;
		}

		const TSharedRef<FConstantValue> ConstantValue = MakeVoxelShared<FConstantValue>();

		// Don't use MakeDynamicValueFactory to skip bConstantPin assert
		ConstantValue->DynamicValue =
			FVoxelDynamicValueFactory(PinData.Compute.ToSharedRef(), PinData.Type, PinData.StatName)
			.AddRef(Node)
			.RunSynchronously()
			.Compute(TerminalGraphInstance.ToSharedRef());

		ensure(!ConstantPins_GameThread.Contains(It.Key));
		ConstantPins_GameThread.Add(It.Key).Add(ConstantValue);
	}

	// Merge variadic pins
	for (const auto& It : Node->GetNodeRuntime().GetVariadicPinNameToPinNames())
	{
		TVoxelArray<TSharedPtr<FConstantValue>> NewConstantValues;
		NewConstantValues.Reserve(It.Value.Num());

		for (const FName Name : It.Value)
		{
			TVoxelArray<TSharedPtr<FConstantValue>> ConstantValues;
			ConstantPins_GameThread.RemoveAndCopyValue(Name, ConstantValues);

			ensure(ConstantValues.Num() == 1);
			NewConstantValues.Add(ConstantValues[0]);
		}

		ensure(!ConstantPins_GameThread.Contains(It.Key));
		ConstantPins_GameThread.Add(It.Key, MoveTemp(NewConstantValues));
	}

	for (const auto& It : ConstantPins_GameThread)
	{
		for (const TSharedPtr<FConstantValue>& ConstantValue : It.Value)
		{
			const auto OnChanged = MakeWeakPtrLambda(this, MakeWeakPtrLambda(ConstantValue, [this, &ConstantValue = *ConstantValue](const FVoxelRuntimePinValue& NewValue)
			{
				checkVoxelSlow(IsInGameThread());

				if (NewValue.IsBuffer())
				{
					const TSharedRef<FVoxelBuffer> NewBuffer = NewValue.Get<FVoxelBuffer>().MakeSharedCopy();
					NewBuffer->Shrink();
					ConstantValue.Value = FVoxelRuntimePinValue::Make(NewBuffer, NewValue.GetType());
				}
				else
				{
					ConstantValue.Value = NewValue;
				}

				OnConstantValueUpdated();
			}));

			ConstantValue->DynamicValue.OnChanged([OnChanged](const FVoxelRuntimePinValue& NewValue)
			{
				RunOnGameThread([OnChanged, NewValue]
				{
					OnChanged(NewValue);
				});
			});
		}
	}
}

void FVoxelExecNodeRuntimeWrapper::OnConstantValueUpdated()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	for (const auto& It : ConstantPins_GameThread)
	{
		for (const TSharedPtr<FConstantValue>& ConstantValue : It.Value)
		{
			if (!ConstantValue->Value.IsValid())
			{
				// Not ready yet
				return;
			}
		}
	}

	TVoxelMap<FName, TVoxelArray<FVoxelRuntimePinValue>> ConstantValues;
	ConstantValues.Reserve(ConstantPins_GameThread.Num());
	for (const auto& It : ConstantPins_GameThread)
	{
		TVoxelArray<FVoxelRuntimePinValue>& Values = ConstantValues.Add_EnsureNew(It.Key);
		Values.Reserve(It.Value.Num());

		for (const TSharedPtr<FConstantValue>& ConstantValue : It.Value)
		{
			Values.Add(ConstantValue->Value);
		}
	}

	if (NodeRuntime_GameThread)
	{
		NodeRuntime_GameThread.Reset();

		LOG_VOXEL(Verbose, "Recreating exec node %s", *Node->GetNodeRef().EdGraphNodeTitle.ToString());
	}

	NodeRuntime_GameThread = Node->CreateSharedExecRuntime(Node);

	if (!NodeRuntime_GameThread)
	{
		// Some nodes don't create on server, like Generate Marching Cube Surface
		return;
	}

	NodeRuntime_GameThread->CallCreate(TerminalGraphInstance.ToSharedRef(), MoveTemp(ConstantValues));
}