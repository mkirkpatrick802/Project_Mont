// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameterPath.h"
#include "VoxelParameterOverridesOwner.h"

class UVoxelGraph;
class FVoxelGraphParametersViewBase;
struct FVoxelParameterValueOverride;

class VOXELGRAPHCORE_API FVoxelGraphParametersViewContext : public TSharedFromThis<FVoxelGraphParametersViewContext>
{
public:
	FVoxelGraphParametersViewContext() = default;

public:
	const FVoxelParameterValueOverride* FindValueOverride(const FVoxelParameterPath& Path) const;

	FVoxelGraphParametersViewBase& FindOrAddParametersView(
		const FVoxelParameterPath& Path,
		const UVoxelGraph& Graph);

public:
	void SetMainOverridesOwner(const FVoxelParameterOverridesOwnerPtr& OverridesOwner);
	void AddGraphOverride(const FVoxelParameterPath& Path, const UVoxelGraph& Graph);

public:
	void ForceEnableOverrides()
	{
		ensure(!bForceEnableMainOverrides);
		bForceEnableMainOverrides = true;
	}

	void AddValueOverrideToIgnore(const FVoxelParameterPath& Path)
	{
		ensure(!MainOverridesOwnerPathToIgnore.IsSet());
		MainOverridesOwnerPathToIgnore = Path;
	}
	void RemoveValueOverrideToIgnore()
	{
		ensure(MainOverridesOwnerPathToIgnore.IsSet());
		MainOverridesOwnerPathToIgnore.Reset();
	}

private:
	FVoxelParameterOverridesOwnerPtr MainOverridesOwner;
	bool bForceEnableMainOverrides = false;
	TOptional<FVoxelParameterPath> MainOverridesOwnerPathToIgnore;

	struct FGraphOverride
	{
		FVoxelParameterPath Path;
		TWeakObjectPtr<const UVoxelGraph> Graph;
	};
	// Highest to lowest priority (graph, then its parent, then parent's parent etc)
	TVoxelArray<FGraphOverride> GraphOverrides;

	// View cache, used to keep alive parameter views even if we change an inline graph
	TVoxelMap<
		TPair<FVoxelParameterPath, TWeakObjectPtr<const UVoxelGraph>>,
		TSharedPtr<FVoxelGraphParametersViewBase>> PathAndGraphToParametersView;
};