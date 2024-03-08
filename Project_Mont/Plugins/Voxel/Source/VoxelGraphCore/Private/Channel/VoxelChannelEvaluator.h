// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelQuery.h"
#include "Channel/VoxelBrush.h"
#include "Buffer/VoxelFloatBuffers.h"

class FVoxelRuntimeChannel;
struct FVoxelSurface;
struct FVoxelChannelDefinition;

class FVoxelChannelEvaluator : public TSharedFromThis<FVoxelChannelEvaluator>
{
public:
	const TSharedRef<const FVoxelRuntimeChannel> Channel;
	const FVoxelChannelDefinition& Definition;
	const FVoxelQuery Query;

	FVoxelChannelEvaluator(
		const TSharedRef<const FVoxelRuntimeChannel>& Channel,
		const FVoxelQuery& Query);

	FVoxelFutureValue Compute();

private:
	TSharedPtr<FVoxelFutureValueStateImpl> FutureValueStateImpl;
	float MinExactDistance = 0.f;
	TVoxelArray<TSharedPtr<const FVoxelBrush>> Brushes;

	void GatherBrushes(
		const FVoxelBox& Bounds,
		FVoxelBrushPriority Priority);

	void ComputeNext(
		bool bIsPreviousDefault,
		const FVoxelRuntimePinValue& PreviousValue,
		int32 BrushIndex) const;

	FVoxelFutureValue GetBrushValue(
		bool bIsPreviousDefault,
		const FVoxelRuntimePinValue& PreviousValue,
		const TSharedRef<const FVoxelBrush>& Brush) const;

private:
	bool TryFilter(
		const TSharedRef<const FVoxelBrush>& Brush,
		FVoxelVectorBuffer& OutPositions,
		FVoxelInt32Buffer& OutIndices,
		FVoxelVectorBuffer& OutFilteredPositions) const;

	FVoxelFutureValue TryGetBrushValue_Buffer(
		bool bIsPreviousDefault,
		const TSharedRef<const FVoxelBuffer>& PreviousBuffer,
		const TSharedRef<const FVoxelBrush>& Brush) const;

	FVoxelFutureValue TryGetBrushValue_Surface(
		bool bIsPreviousDefault,
		const TSharedRef<const FVoxelSurface>& PreviousSurface,
		const TSharedRef<const FVoxelBrush>& Brush) const;
};