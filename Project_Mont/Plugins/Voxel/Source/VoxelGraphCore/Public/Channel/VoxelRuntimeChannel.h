// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"
#include "VoxelTransformRef.h"
#include "Channel/VoxelBrush.h"
#include "Channel/VoxelChannelDefinition.h"

class FVoxelWorldChannel;

class VOXELGRAPHCORE_API FVoxelRuntimeChannel : public TSharedFromThis<FVoxelRuntimeChannel>
{
public:
	const TSharedRef<FVoxelWorldChannel> WorldChannel;
	const FVoxelChannelDefinition Definition;
	const FVoxelTransformRef RuntimeLocalToWorld;

	VOXEL_COUNT_INSTANCES();

	FVoxelFutureValue Get(const FVoxelQuery& Query) const;

	template<typename T>
	TVoxelFutureValue<T> Get(const FVoxelQuery& Query) const
	{
		const FVoxelFutureValue Value = Get(Query);
		if (!ensure(Value.IsValid()) ||
			!ensure(Value.GetParentType().Is<T>()))
		{
			return {};
		}
		return TVoxelFutureValue<T>(Value);
	}

public:
	static FVoxelBrushPriority GetFullPriority(
		int32 Priority,
		const FString& GraphPath,
		const FString* NodeId,
		const FString& InstancePath);

	static FVoxelBrushPriority GetPriority(int32 Priority);

private:
	const TSharedRef<FVoxelDependency> Dependency;
	mutable FVoxelCriticalSection CriticalSection;

	struct FRuntimeBrush
	{
		const TSharedRef<const FVoxelBrush> Brush;
		const FVoxelTransformRef BrushToRuntime;
		const FVoxelBrushPriority Priority;
		TOptional<FVoxelBox> RuntimeBounds_RequiresLock;

		FRuntimeBrush(
			const TSharedRef<const FVoxelBrush>& Brush,
			const FVoxelTransformRef& BrushToRuntime)
			: Brush(Brush)
			, BrushToRuntime(BrushToRuntime)
			, Priority(Brush->Priority)
		{
		}
	};
	TVoxelMap<FVoxelBrushId, TSharedPtr<FRuntimeBrush>> RuntimeBrushes_RequiresLock;

	FVoxelRuntimeChannel(
		const TSharedRef<FVoxelWorldChannel>& WorldChannel,
		const FVoxelTransformRef& RuntimeLocalToWorld);

	void AddBrush(
		FVoxelBrushId BrushId,
		const TSharedRef<const FVoxelBrush>& Brush);

	void RemoveBrush(
		FVoxelBrushId BrushId,
		const TSharedRef<const FVoxelBrush>& Brush);

	friend class FVoxelWorldChannel;
	friend class FVoxelChannelEvaluator;
};