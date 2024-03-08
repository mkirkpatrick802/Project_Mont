// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelTransformRef.h"
#include "Channel/VoxelBrush.h"
#include "Buffer/VoxelBaseBuffers.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "VoxelGraphBlueprintLibrary.generated.h"

class AVoxelActor;
class UVoxelGraph;
class FVoxelBrushRef;
struct FVoxelBrushPriority;

UCLASS()
class VOXELGRAPHCORE_API UVoxelGraphBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, DisplayName = "Query Voxel Channel", CustomThunk, Category = "Voxel", meta = (WorldContext = "WorldContextObject", CustomStructureParam = "Value", BlueprintInternalUseOnly = "true", AdvancedDisplay = "MaxPriority, LOD, GradientStep"))
	static void K2_QueryVoxelChannel(
		int32& Value,
		UObject* WorldContextObject,
		FName Channel,
		FVector Position,
		int32 MaxPriority = 32767,
		int32 LOD = 0,
		float GradientStep = 100.f)
	{
		checkf(false, TEXT("Use QueryVoxelChannel instead"));
	}
	DECLARE_FUNCTION(execK2_QueryVoxelChannel);

	UFUNCTION(BlueprintCallable, DisplayName = "Multi Query Voxel Channel", CustomThunk, Category = "Voxel", meta = (WorldContext = "WorldContextObject", CustomStructureParam = "Value", BlueprintInternalUseOnly = "true", AdvancedDisplay = "MaxPriority, LOD, GradientStep"))
	static void K2_MultiQueryVoxelChannel(
		int32& Value,
		UObject* WorldContextObject,
		FName Channel,
		TArray<FVector> Positions,
		int32 MaxPriority = 32767,
		int32 LOD = 0,
		float GradientStep = 100.f)
	{
		checkf(false, TEXT("Use QueryVoxelChannel instead"));
	}
	DECLARE_FUNCTION(execK2_MultiQueryVoxelChannel);

public:
	UFUNCTION(BlueprintCallable, DisplayName = "Query Voxel Graph Output", CustomThunk, Category = "Voxel", meta = (WorldContext = "WorldContextObject", CustomStructureParam = "Value", BlueprintInternalUseOnly = "true"))
	static void K2_QueryVoxelGraphOutput(
		int32& Value,
		UObject* WorldContextObject,
		UVoxelGraph* Graph,
		FName Name)
	{
		unimplemented();
	}
	DECLARE_FUNCTION(execK2_QueryVoxelGraphOutput);

public:
	template<typename T>
	static bool QueryChannel(
		UObject* WorldContextObject,
		const FName Channel,
		const FVector& Position,
		int32 MaxPriority,
		T& OutValue)
	{
		return UVoxelGraphBlueprintLibrary::QueryChannel(
			WorldContextObject,
			Channel,
			Position,
			MaxPriority,
			0,
			100.f,
			OutValue);
	}
	template<typename T>
	static bool MultiQueryChannel(
		UObject* WorldContextObject,
		const FName Channel,
		const TArray<FVector>& Positions,
		const int32 MaxPriority,
		TArray<T>& OutValues)
	{
		return UVoxelGraphBlueprintLibrary::MultiQueryChannel(
			WorldContextObject,
			Channel,
			Positions,
			MaxPriority,
			0,
			100.f,
			OutValues);
	}

public:
	template<typename T>
	static bool QueryChannel(
		UObject* WorldContextObject,
		const FName Channel,
		const FVector& Position,
		const int32 MaxPriority,
		const int32 LOD,
		const float GradientStep,
		T& OutValue)
	{
		VOXEL_FUNCTION_COUNTER();

		const FVoxelRuntimePinValue Value = QueryChannel(
			WorldContextObject,
			Channel,
			TArray<FVector3f>({ FVector3f(Position) }),
			MaxPriority,
			LOD,
			GradientStep);

		if (!Value.IsValid())
		{
			return false;
		}

		using ValueType = std::decay_t<typename TRemoveObjectPointer<std::remove_pointer_t<T>>::Type>;

		const FVoxelPinType ExposedType = Value.GetType().GetExposedType();
		if (!ExposedType.CanBeCastedTo<ValueType>())
		{
			return false;
		}

		const FVoxelPinValue ExposedValue = FVoxelPinType::MakeExposedValue(Value, false);
		if (!ensure(ExposedValue.IsValid()))
		{
			return false;
		}

		OutValue = ExposedValue.Get<ValueType>();
		return true;
	}

	template<typename T>
	static bool MultiQueryChannel(
		UObject* WorldContextObject,
		const FName Channel,
		const TArray<FVector>& Positions,
		const int32 MaxPriority,
		const int32 LOD,
		const float GradientStep,
		TArray<T>& OutValues)
	{
		VOXEL_FUNCTION_COUNTER_NUM(Positions.Num(), 1);

		const FVoxelRuntimePinValue Value = QueryChannel(
			WorldContextObject,
			Channel,
			TArray<FVector3f>(Positions),
			MaxPriority,
			LOD,
			GradientStep);

		if (!Value.IsValid() ||
			!Value.IsBuffer())
		{
			return false;
		}

		// Fast path for terminal buffers (float/int etc)
		if constexpr (!std::is_same_v<typename TVoxelBufferType<T>::Type, void>)
		{
			using BufferType = typename TVoxelBufferType<T>::Type;

			const BufferType& Buffer = Value.Get<BufferType>();
			const TVoxelBufferStorage<T>& Storage = Buffer.GetStorage();

			FVoxelUtilities::SetNumFast(OutValues, Storage.Num());
			Storage.CopyTo(OutValues);
			return true;
		}

		using ValueType = std::decay_t<typename TRemoveObjectPointer<std::remove_pointer_t<T>>::Type>;

		const FVoxelPinType ExposedInnerType = Value.GetType().GetInnerType().GetExposedType();
		if (!ExposedInnerType.CanBeCastedTo<ValueType>())
		{
			return false;
		}

		const FVoxelPinValue ExposedValue = FVoxelPinType::MakeExposedValue(Value, true);
		if (!ensure(ExposedValue.IsValid()))
		{
			return false;
		}

		for (const FVoxelTerminalPinValue& ExposedTerminalValue : ExposedValue.GetArray())
		{
			OutValues.Add(ExposedTerminalValue.Get<ValueType>());
		}
		return true;
	}

	// Will return a buffer of the same size as Positions
	// If Positions.Num() == 1, you will want to do
	// QueryChannel(...).Get<FVoxelFloatBuffer>().GetConstant()
	static FVoxelRuntimePinValue QueryChannel(
		const UObject* WorldContextObject,
		FName ChannelName,
		TConstVoxelArrayView<FVector3f> Positions,
		int32 MaxPriority = 32767,
		int32 LOD = 0,
		float GradientStep = 100.f);

public:
	static TSharedPtr<FVoxelBrushRef> RegisterBrush(
		const UWorld* World,
		FName Channel,
		FName DebugName,
		const FVoxelPinType& Type,
		FVoxelComputeValue Compute,
		const FVoxelBox& LocalBounds,
		const FVoxelTransformRef& LocalToWorld,
		const FVoxelBrushPriority& Priority);

	template<typename T>
	static TSharedPtr<FVoxelBrushRef> RegisterBrush(
		const UWorld* World,
		const FName Channel,
		const FName DebugName,
		TVoxelComputeValue<T> Compute,
		const FVoxelBox& LocalBounds = FVoxelBox::Infinite,
		const FVoxelTransformRef& LocalToWorld = FVoxelTransformRef::Identity(),
		const FVoxelBrushPriority& Priority = FVoxelBrushPriority::Max())
	{
		return UVoxelGraphBlueprintLibrary::RegisterBrush(
			World,
			Channel,
			DebugName,
			FVoxelPinType::Make<T>(),
			MoveTemp(ReinterpretCastRef<FVoxelComputeValue>(Compute)),
			LocalBounds,
			LocalToWorld,
			Priority);
	}

public:
	UFUNCTION(BlueprintCallable, Category = "Voxel", meta = (WorldContext = "WorldContextObject"))
	static void RegisterVoxelChannel(
		const UObject* WorldContextObject,
		FName ChannelName,
		FVoxelPinType Type,
		FVoxelPinValue DefaultValue);
};