// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphParametersViewContext.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelGraph.h"
#include "VoxelGraphParametersView.h"

const FVoxelParameterValueOverride* FVoxelGraphParametersViewContext::FindValueOverride(const FVoxelParameterPath& Path) const
{
	VOXEL_FUNCTION_COUNTER();

	// First check the main overrides
	// This is typically a voxel actor or a graph
	const FVoxelParameterValueOverride* MainValueOverride = INLINE_LAMBDA -> const FVoxelParameterValueOverride*
	{
		const IVoxelParameterOverridesOwner* OverridesOwner = MainOverridesOwner.Get();
		if (!ensure(OverridesOwner))
		{
			return nullptr;
		}

		if (MainOverridesOwnerPathToIgnore.IsSet() &&
			MainOverridesOwnerPathToIgnore.GetValue() == Path)
		{
			return nullptr;
		}

		const FVoxelParameterValueOverride* ValueOverride = OverridesOwner->GetPathToValueOverride().Find(Path);
		if (!ValueOverride)
		{
			return nullptr;
		}

		if (!bForceEnableMainOverrides &&
			!ValueOverride->bEnable)
		{
			return nullptr;
		}

		return ValueOverride;
	};
	if (MainValueOverride)
	{
		return MainValueOverride;
	}

	for (const FGraphOverride& GraphOverride : GraphOverrides)
	{
		if (!Path.StartsWith(GraphOverride.Path))
		{
			continue;
		}
		const FVoxelParameterPath RelativePath = Path.MakeRelativeTo(GraphOverride.Path);

		const UVoxelGraph* Graph = GraphOverride.Graph.Get();
		if (!ensure(Graph))
		{
			continue;
		}

		const FVoxelParameterValueOverride* ValueOverride = Graph->GetPathToValueOverride().Find(RelativePath);
		if (!ValueOverride ||
			!ValueOverride->bEnable)
		{
			continue;
		}

		return ValueOverride;
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelGraphParametersViewBase& FVoxelGraphParametersViewContext::FindOrAddParametersView(
	const FVoxelParameterPath& Path,
	const UVoxelGraph& Graph)
{
	TSharedPtr<FVoxelGraphParametersViewBase>& ParametersView = PathAndGraphToParametersView.FindOrAdd({ Path, &Graph });
	if (ParametersView)
	{
		return *ParametersView;
	}

	for (const UVoxelGraph* BaseGraph : Graph.GetBaseGraphs())
	{
		AddGraphOverride(Path, *BaseGraph);
	}

	ParametersView = MakeVoxelShared<FVoxelGraphParametersViewBase>();
	ParametersView->Initialize(*this, Path, Graph);

	return *ParametersView;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphParametersViewContext::SetMainOverridesOwner(const FVoxelParameterOverridesOwnerPtr& OverridesOwner)
{
	ensure(!MainOverridesOwner.IsValid());
	MainOverridesOwner = OverridesOwner;

	if (const UVoxelGraph* Graph = Cast<UVoxelGraph>(OverridesOwner.GetObject()))
	{
		// Avoid duplicate overrides
		ensure(GraphOverrides[0].Path.IsEmpty());
		ensure(GraphOverrides[0].Graph == Graph);
		GraphOverrides.RemoveAt(0);
	}
}

void FVoxelGraphParametersViewContext::AddGraphOverride(const FVoxelParameterPath& Path, const UVoxelGraph& Graph)
{
	GraphOverrides.Add(FGraphOverride
	{
		Path,
		&Graph
	});
}