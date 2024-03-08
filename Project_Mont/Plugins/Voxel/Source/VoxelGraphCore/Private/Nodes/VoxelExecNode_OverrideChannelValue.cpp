// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelExecNode_OverrideChannelValue.h"
#include "Nodes/VoxelNode_QueryChannel.h"
#include "VoxelSurface.h"
#include "VoxelTerminalGraph.h"
#include "VoxelTerminalGraphRuntime.h"
#include "Channel/VoxelWorldChannel.h"
#include "Channel/VoxelRuntimeChannel.h"
#include "Channel/VoxelChannelManager.h"
#include "EdGraph/EdGraphNode.h"

FVoxelExecNode_OverrideChannelValue::FVoxelExecNode_OverrideChannelValue()
{
	GetPin(ValuePin).SetType(FVoxelPinType::Make<FVoxelSurface>());
}

TVoxelUniquePtr<FVoxelExecNodeRuntime> FVoxelExecNode_OverrideChannelValue::CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const
{
	return MakeVoxelUnique<FVoxelExecNodeRuntime_OverrideChannelValue>(SharedThis);
}

#if WITH_EDITOR
void FVoxelExecNode_OverrideChannelValue::FDefinition::Initialize(UEdGraphNode& EdGraphNode)
{
	GVoxelChannelManager->OnChannelDefinitionsChanged_GameThread.Add(MakeWeakPtrDelegate(this, [this, EdGraphNode = MakeWeakObjectPtr(&EdGraphNode)]
	{
		if (!ensure(EdGraphNode.IsValid()))
		{
			return;
		}

		EdGraphNode->ReconstructNode();
	}));
}

bool FVoxelExecNode_OverrideChannelValue::FDefinition::OnPinDefaultValueChanged(const FName PinName, const FVoxelPinValue& NewDefaultValue)
{
	if (PinName != Node.ChannelPin ||
		!NewDefaultValue.IsValid() ||
		!NewDefaultValue.Is<FVoxelChannelName>())
	{
		return false;
	}

	const FVoxelChannelName ChannelName = NewDefaultValue.Get<FVoxelChannelName>();
	const TOptional<FVoxelChannelDefinition> ChannelDefinition = GVoxelChannelManager->FindChannelDefinition(ChannelName.Name);

	if (!ChannelDefinition ||
		ChannelDefinition->Type == Node.GetPin(Node.ValuePin).GetType())
	{
		return false;
	}

	Node.GetPin(Node.ValuePin).SetType(ChannelDefinition->Type);
	return true;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelExecNodeRuntime_OverrideChannelValue::Create()
{
	VOXEL_FUNCTION_COUNTER();

	const FName ChannelName = GetConstantPin(Node.ChannelPin).Name;
	const int32 Priority = GetConstantPin(Node.PriorityPin);
	const TSharedRef<const FVoxelBounds> Bounds = GetConstantPin(Node.BoundsPin);

	const FVoxelQueryScope Scope(nullptr, &GetTerminalGraphInstance().Get());

	const TSharedRef<FVoxelWorldChannelManager> ChannelManager = FVoxelWorldChannelManager::Get(GetWorld());
	const TSharedPtr<FVoxelWorldChannel> Channel = ChannelManager->FindChannel(ChannelName);
	if (!Channel)
	{
		VOXEL_MESSAGE(Error, "{0}: No channel named {1} found. Valid names: {2}",
			this,
			ChannelName,
			ChannelManager->GetValidChannelNames());
		return;
	}

	const FString NodeId = GetNodeRef().NodeId.ToString();

	const FVoxelBrushPriority FullPriority = FVoxelRuntimeChannel::GetFullPriority(
		Priority,
		GetGraphPath(),
		&NodeId,
		GetInstancePath());

	if (!Bounds->IsValid())
	{
		VOXEL_MESSAGE(Error, "{0}: Invalid bounds", this);
		return;
	}

	// Shouldn't need invalidation
	ensure(Bounds->GetLocalToWorld() == GetRuntimeInfo()->GetLocalToWorld());
	const FVoxelBox LocalBounds = Bounds->GetBox_NoDependency(GetRuntimeInfo()->GetLocalToWorld());

	const TSharedRef<FVoxelBrush> Brush = MakeVoxelShared<FVoxelBrush>(
		FName(GetInstanceName()),
		FullPriority,
		LocalBounds,
		GetLocalToWorld(),
		[
			FullPriority,
			NodeRef = GetNodeRef(),
			ComputeValue = GetNodeRuntime().GetCompute(Node.ValuePin, GetTerminalGraphInstance()),
			Definition = Channel->Definition](const FVoxelQuery& Query) -> FVoxelFutureValue
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			FVoxelBrushPriorityQueryParameter::AddMaxPriority(*Parameters, Definition.Name, FullPriority);
			const FVoxelFutureValue Value = (*ComputeValue)(Query.MakeNewQuery(Parameters));

			return
				MakeVoxelTask()
				.Dependency(Value)
				.Execute(Definition.Type, [Value, Definition, NodeRef = NodeRef]
				{
					const FVoxelRuntimePinValue SharedValue = Value.GetValue_CheckCompleted();
					if (!SharedValue.CanBeCastedTo(Definition.Type))
					{
						VOXEL_MESSAGE(Error, "{0}: Invalid value type {1}. Should be the same as the type of channel being queried ({2}): {3}",
							NodeRef,
							SharedValue.GetType().ToString(),
							Definition.Name,
							Definition.Type.ToString());
						return FVoxelRuntimePinValue(Definition.Type);
					}
					return SharedValue;
				});
		});

	Channel->AddBrush(Brush, BrushRef);
}

void FVoxelExecNodeRuntime_OverrideChannelValue::Destroy()
{
	BrushRef = {};
}