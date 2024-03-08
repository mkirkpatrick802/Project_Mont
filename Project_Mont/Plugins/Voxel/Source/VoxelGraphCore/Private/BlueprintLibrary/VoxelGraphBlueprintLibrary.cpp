// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "BlueprintLibrary/VoxelGraphBlueprintLibrary.h"
#include "VoxelActor.h"
#include "VoxelGraph.h"
#include "VoxelTaskHelpers.h"
#include "VoxelDependencyTracker.h"
#include "Buffer/VoxelFloatBuffers.h"
#include "Channel/VoxelWorldChannel.h"
#include "Channel/VoxelRuntimeChannel.h"
#include "Channel/VoxelChannelManager.h"
#include "VoxelTerminalGraphInstance.h"
#include "VoxelPositionQueryParameter.h"
#include "Nodes/VoxelNode_QueryChannel.h"

DEFINE_FUNCTION(UVoxelGraphBlueprintLibrary::execK2_QueryVoxelChannel)
{
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* Property = Stack.MostRecentProperty;
	uint8* PropertyAddress = Stack.MostRecentPropertyAddress;

	P_GET_OBJECT(UObject, WorldContextObject);
	P_GET_PROPERTY(FNameProperty, Channel);
	P_GET_STRUCT(FVector, Position);
	P_GET_PROPERTY(FIntProperty, MaxPriority);
	P_GET_PROPERTY(FIntProperty, LOD);
	P_GET_PROPERTY(FFloatProperty, GradientStep);

	P_FINISH;

	if (!ensure(Property) ||
		!ensure(PropertyAddress))
	{
		VOXEL_MESSAGE(Error, "Failed to resolve the Value parameter");
		return;
	}

	FVoxelRuntimePinValue Value = QueryChannel(
		WorldContextObject,
		Channel,
		TArray{ FVector3f(Position) },
		MaxPriority,
		LOD,
		GradientStep);
	if (!Value.IsValid())
	{
		return;
	}

	const FVoxelPinType ValueType = Value.GetType().GetExposedType();
	const FVoxelPinType ExpectedType(*Property);
	if (!ValueType.CanBeCastedTo_K2(ExpectedType))
	{
		VOXEL_MESSAGE(Error, "Channel returned a value with type {0}, but the blueprint Value pin has type {1}",
			ValueType.ToString(),
			ExpectedType.ToString());
		return;
	}

	const FVoxelPinValue ExposedValue = FVoxelPinType::MakeExposedValue(Value, false);
	if (!ExposedValue.IsValid())
	{
		VOXEL_MESSAGE(Error, "Failed to query value");
		return;
	}

	ExposedValue.ExportToProperty(*Property, PropertyAddress);
}

DEFINE_FUNCTION(UVoxelGraphBlueprintLibrary::execK2_MultiQueryVoxelChannel)
{
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* Property = Stack.MostRecentProperty;
	uint8* PropertyAddress = Stack.MostRecentPropertyAddress;

	P_GET_OBJECT(UObject, WorldContextObject);
	P_GET_PROPERTY(FNameProperty, Channel);
	P_GET_TARRAY(FVector, Positions);
	P_GET_PROPERTY(FIntProperty, MaxPriority);
	P_GET_PROPERTY(FIntProperty, LOD);
	P_GET_PROPERTY(FFloatProperty, GradientStep);

	P_FINISH;

	if (!ensure(Property) ||
		!ensure(PropertyAddress))
	{
		VOXEL_MESSAGE(Error, "Failed to resolve the Value parameter");
		return;
	}

	FVoxelRuntimePinValue Value = QueryChannel(
		WorldContextObject,
		Channel,
		TArray<FVector3f>(Positions),
		MaxPriority,
		LOD,
		GradientStep);
	if (!Value.IsValid())
	{
		return;
	}

	const FVoxelPinType ExpectedType(*Property);

	if (Value.GetType().IsBuffer() &&
		!ExpectedType.IsBuffer())
	{
		// Ideally we should check whether Value is not an array, but that's not possible here
		// since we don't maintain array flags at runtime

		if (!Value.Get<FVoxelBuffer>().IsConstant())
		{
			VOXEL_MESSAGE(Error, "Cannot extract constant from non-constant buffer");
			return;
		}

		Value = Value.Get<FVoxelBuffer>().GetGenericConstant();
	}

	FVoxelPinType ValueType;
	if (ExpectedType.IsBufferArray() &&
		Value.IsBuffer())
	{
		ValueType = Value.GetType().WithBufferArray(true).GetExposedType();
	}
	else
	{
		ValueType = Value.GetType().GetExposedType();
	}

	if (!ValueType.CanBeCastedTo_K2(ExpectedType))
	{
		VOXEL_MESSAGE(Error, "Channel returned a value with type {0}, but the blueprint Value pin has type {1}",
			ValueType.ToString(),
			ExpectedType.ToString());
		return;
	}

	const FVoxelPinValue ExposedValue = FVoxelPinType::MakeExposedValue(Value, ExpectedType.IsBufferArray());
	if (!ExposedValue.IsValid())
	{
		VOXEL_MESSAGE(Error, "Failed to query value");
		return;
	}

	ExposedValue.ExportToProperty(*Property, PropertyAddress);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_FUNCTION(UVoxelGraphBlueprintLibrary::execK2_QueryVoxelGraphOutput)
{
	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* Property = Stack.MostRecentProperty;
	uint8* PropertyAddress = Stack.MostRecentPropertyAddress;

	P_GET_OBJECT(UObject, WorldContextObject);
	P_GET_OBJECT(UVoxelGraph, Graph);
	P_GET_PROPERTY(FNameProperty, Name);

	P_FINISH;

	if (!ensure(Property) ||
		!ensure(PropertyAddress))
	{
		VOXEL_MESSAGE(Error, "Failed to resolve the Value parameter");
		return;
	}

	// TODO: Implement querying
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimePinValue UVoxelGraphBlueprintLibrary::QueryChannel(
	const UObject* WorldContextObject,
	const FName ChannelName,
	const TConstVoxelArrayView<FVector3f> Positions,
	const int32 MaxPriority,
	const int32 LOD,
	const float GradientStep)
{
	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num(), 1);
	ensure(IsInGameThread());

	if (!WorldContextObject)
	{
		VOXEL_MESSAGE(Error, "WorldContextObject is null");
		return {};
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		VOXEL_MESSAGE(Error, "World is null");
		return {};
	}

	const TSharedPtr<FVoxelRuntimeChannel> Channel = FVoxelWorldChannelManager::FindRuntimeChannel(World, ChannelName);
	if (!Channel)
	{
		VOXEL_MESSAGE(Error, "No channel {0} found. Valid channels: {1}",
			ChannelName,
			FVoxelWorldChannelManager::Get(World)->GetValidChannelNames());
		return {};
	}

	FVoxelFloatBufferStorage X; X.Allocate(Positions.Num());
	FVoxelFloatBufferStorage Y; Y.Allocate(Positions.Num());
	FVoxelFloatBufferStorage Z; Z.Allocate(Positions.Num());
	for (int32 Index = 0; Index < Positions.Num(); Index++)
	{
		X[Index] = Positions[Index].X;
		Y[Index] = Positions[Index].Y;
		Z[Index] = Positions[Index].Z;
	}

	const TSharedRef<FVoxelRuntimeInfo> RuntimeInfo =
		FVoxelRuntimeInfoBase::MakeFromWorld(World)
		.EnableParallelTasks()
		.MakeRuntimeInfo_RequiresDestroy();
	ON_SCOPE_EXIT
	{
		RuntimeInfo->Destroy();
	};

	const TSharedRef<FVoxelTerminalGraphInstance> TerminalGraphInstance = FVoxelTerminalGraphInstance::MakeDummy(RuntimeInfo);

	FString Error;
	FVoxelFutureValue Future;
	const bool bSuccess = FVoxelTaskHelpers::TryRunSynchronouslyGeneric(TerminalGraphInstance, [&]
	{
		const TSharedRef<FVoxelQueryParameters> Parameters = MakeVoxelShared<FVoxelQueryParameters>();
		Parameters->Add<FVoxelPositionQueryParameter>().Initialize(FVoxelVectorBuffer::Make(X, Y, Z));
		Parameters->Add<FVoxelLODQueryParameter>().LOD = LOD;
		Parameters->Add<FVoxelGradientStepQueryParameter>().Step = GradientStep;
		FVoxelBrushPriorityQueryParameter::AddMaxPriority(
			*Parameters,
			ChannelName,
			FVoxelRuntimeChannel::GetPriority(MaxPriority));

		const FVoxelQuery Query = FVoxelQuery::Make(
			TerminalGraphInstance,
			Parameters,
			FVoxelDependencyTracker::Create("DependencyTracker"));

		const FVoxelFutureValue FutureValue = Channel->Get(Query);
		if (!ensure(FutureValue.IsValid()))
		{
			return;
		}

		Future =
			MakeVoxelTask()
			.Dependency(FutureValue)
			.Execute(Channel->Definition.Type, [FutureValue]
			{
				return FutureValue.GetValue_CheckCompleted();
			});
	}, &Error);

	if (!bSuccess)
	{
		VOXEL_MESSAGE(Error, "Failed to query value: {0}", Error);
		return {};
	}

	if (!ensure(Future.IsValid()) ||
		!ensure(Future.IsComplete()))
	{
		VOXEL_MESSAGE(Error, "Failed to query value");
		return {};
	}

	return Future.GetValue_CheckCompleted();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FVoxelBrushRef> UVoxelGraphBlueprintLibrary::RegisterBrush(
	const UWorld* World,
	const FName Channel,
	const FName DebugName,
	const FVoxelPinType& Type,
	FVoxelComputeValue Compute,
	const FVoxelBox& LocalBounds,
	const FVoxelTransformRef& LocalToWorld,
	const FVoxelBrushPriority& Priority)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FVoxelWorldChannelManager> ChannelManager = FVoxelWorldChannelManager::Get(World);
	const TSharedPtr<FVoxelWorldChannel> WorldChannel = ChannelManager->FindChannel(Channel);
	if (!ensure(WorldChannel) ||
		!ensure(WorldChannel->Definition.Type == Type))
	{
		return nullptr;
	}

	const TSharedRef<FVoxelBrush> Brush = MakeVoxelShared<FVoxelBrush>(
		DebugName,
		Priority,
		LocalBounds,
		LocalToWorld,
		MoveTemp(Compute));

	TSharedPtr<FVoxelBrushRef> BrushRef;
	WorldChannel->AddBrush(Brush, BrushRef);
	return BrushRef;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphBlueprintLibrary::RegisterVoxelChannel(
	const UObject* WorldContextObject,
	const FName ChannelName,
	const FVoxelPinType Type,
	FVoxelPinValue DefaultValue)
{
	VOXEL_FUNCTION_COUNTER()
	ensure(IsInGameThread());

	if (!WorldContextObject)
	{
		VOXEL_MESSAGE(Error, "WorldContextObject is null");
		return;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		VOXEL_MESSAGE(Error, "World is null");
		return;
	}

	if (!Type.IsValid())
	{
		VOXEL_MESSAGE(Error, "Type is invalid");
		return;
	}

	if (!DefaultValue.IsValid())
	{
		DefaultValue = FVoxelPinValue(Type.GetExposedType());
	}

	if (DefaultValue.GetType() != Type.GetExposedType())
	{
		VOXEL_MESSAGE(Error, "DefaultValue has type {0}, but should have type {1}", DefaultValue.GetType().ToString(), Type.GetExposedType().ToString());
		return;
	}

	if (GVoxelChannelManager->FindChannelDefinition(ChannelName))
	{
		VOXEL_MESSAGE(Error, "Channel {0} is already registered globally", ChannelName);
		return;
	}

	const TSharedRef<FVoxelWorldChannelManager> ChannelManager = FVoxelWorldChannelManager::Get(World);
	if (ChannelManager->FindChannel(ChannelName))
	{
		VOXEL_MESSAGE(Error, "Channel {0} is already registered", ChannelName);
		return;
	}

	const FVoxelChannelDefinition ChannelDefinition
	{
		ChannelName,
		Type,
		FVoxelPinType::MakeRuntimeValue(Type, DefaultValue)
	};

	if (!ChannelManager->RegisterChannel(ChannelDefinition))
	{
		VOXEL_MESSAGE(Error, "Failed to register channel {0}", ChannelName);
	}
}