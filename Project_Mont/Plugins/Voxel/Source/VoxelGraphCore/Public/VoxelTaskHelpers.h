// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"

class FVoxelTerminalGraphInstance;

class VOXELGRAPHCORE_API FVoxelTaskHelpers
{
public:
	static void StartAsyncTaskGeneric(
		FName Name,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		const FVoxelPinType& Type,
		TVoxelUniqueFunction<FVoxelFutureValue()> MakeFuture,
		TFunction<void(const FVoxelRuntimePinValue&)> Callback);

	template<typename T>
	static void StartAsyncTask(
		const FName Name,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		TVoxelUniqueFunction<TVoxelFutureValue<T>()> MakeFuture,
		TFunction<void(const typename TChooseClass<VoxelPassByValue<T>, T, TSharedRef<const T>>::Result&)> Callback)
	{
		FVoxelTaskHelpers::StartAsyncTaskGeneric(
			Name,
			TerminalGraphInstance,
			FVoxelPinType::Make<T>(),
			MoveTemp(ReinterpretCastRef<TVoxelUniqueFunction<FVoxelFutureValue()>>(MakeFuture)),
			[Callback = MoveTemp(Callback)](const FVoxelRuntimePinValue& Value)
			{
				if constexpr (VoxelPassByValue<T>)
				{
					Callback(Value.Get<T>());
				}
				else
				{
					Callback(Value.GetSharedStruct<T>());
				}
			});
	}

public:
	static bool TryRunSynchronouslyGeneric(
		const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		TFunctionRef<void()> Lambda,
		FString* OutError = nullptr);

	template<typename LambdaType, typename T = typename decltype(DeclVal<LambdaType>()())::Type>
	static TOptional<T> TryRunSynchronously(
		const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		LambdaType&& Lambda,
		FString* OutError = nullptr)
	{
		TVoxelFutureValue<T> Future;

		if (!FVoxelTaskHelpers::TryRunSynchronouslyGeneric(TerminalGraphInstance, [&]
			{
				Future = Lambda();
			}, OutError))
		{
			return {};
		}

		if (!ensure(Future.IsValid()) ||
			!ensure(Future.IsComplete()))
		{
			return {};
		}

		return Future.Get_CheckCompleted();
	}
};