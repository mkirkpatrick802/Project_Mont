// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameter.h"
#include "VoxelInlineGraph.h"
#include "VoxelParameterPath.h"

class FVoxelGraphParametersViewBase;
class FVoxelGraphParametersViewContext;

class VOXELGRAPHCORE_API FVoxelParameterView
{
public:
	FVoxelGraphParametersViewContext& Context;
	const FVoxelParameterPath Path;

	FVoxelParameterView(
		FVoxelGraphParametersViewContext& Context,
		const FVoxelParameterPath& Path,
		const FVoxelParameter& Parameter);
	UE_NONCOPYABLE(FVoxelParameterView)

public:
	FORCEINLINE FGuid GetGuid() const
	{
		return Path.Leaf();
	}
	FORCEINLINE const FVoxelParameter& GetParameter() const
	{
		return Parameter;
	}
	FORCEINLINE FName GetName() const
	{
		return Parameter.Name;
	}
	FORCEINLINE bool IsInlineGraph() const
	{
		return Parameter.Type.Is<FVoxelInlineGraph>();
	}

public:
	FVoxelPinType GetType() const;
	// Value.Type is GetType().GetExposedType()
	FVoxelPinValue GetValue() const;

	TConstVoxelArrayView<FVoxelParameterView*> GetChildren() const;
	FVoxelParameterView* FindByGuid(const FGuid& Guid) const;
	FVoxelParameterView* FindByName(FName Name) const;

public:
	static TVoxelArray<TVoxelArray<FVoxelParameterView*>> GetCommonChildren(TConstVoxelArrayView<FVoxelParameterView*> ParameterViews);

private:
	const FVoxelParameter Parameter;

	FVoxelGraphParametersViewBase* GetInlineGraphParametersView() const;
};