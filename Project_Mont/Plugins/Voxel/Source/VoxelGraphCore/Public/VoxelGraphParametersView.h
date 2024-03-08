// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"

class UVoxelGraph;
class FVoxelParameterView;
class FVoxelGraphParametersViewContext;
struct FVoxelParameterPath;

class VOXELGRAPHCORE_API FVoxelGraphParametersViewBase
{
public:
	FVoxelGraphParametersViewBase() = default;

	void Initialize(
		FVoxelGraphParametersViewContext& Context,
		const FVoxelParameterPath& BasePath,
		const UVoxelGraph& Graph);

public:
	FORCEINLINE TConstVoxelArrayView<FVoxelParameterView*> GetChildren() const
	{
		return Children;
	}
	FORCEINLINE FVoxelParameterView* FindByGuid(const FGuid& Guid) const
	{
		return GuidToParameterView.FindRef(Guid);
	}
	FORCEINLINE FVoxelParameterView* FindByName(const FName Name) const
	{
		return NameToParameterView.FindRef(Name);
	}

public:
	static TVoxelArray<TVoxelArray<FVoxelParameterView*>> GetCommonChildren(TConstVoxelArrayView<const FVoxelGraphParametersViewBase*> ParametersViews);

private:
	TVoxelArray<TSharedPtr<FVoxelParameterView>> ChildrenRefs;
	TVoxelArray<FVoxelParameterView*> Children;
	TVoxelMap<FGuid, FVoxelParameterView*> GuidToParameterView;
	TVoxelMap<FName, FVoxelParameterView*> NameToParameterView;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelGraphParametersView : public FVoxelGraphParametersViewBase
{
public:
	explicit FVoxelGraphParametersView(const UVoxelGraph& Graph);

public:
	FORCEINLINE FVoxelGraphParametersViewContext& GetContext() const
	{
		return *Context;
	}

public:
	FVoxelParameterView* FindChild(const FVoxelParameterPath& Path) const;

private:
	const TSharedRef<FVoxelGraphParametersViewContext> Context;

	using FVoxelGraphParametersViewBase::Initialize;
};