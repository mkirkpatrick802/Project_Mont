// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelFutureValue.h"
#include "VoxelParameterPath.h"
#include "VoxelParameterOverridesOwner.h"

class UVoxelGraph;
class FVoxelDependency;
class FVoxelGraphEvaluatorRef;

class VOXELGRAPHCORE_API FVoxelParameterOverridesRuntime : public TSharedFromThis<FVoxelParameterOverridesRuntime>
{
public:
	static TSharedRef<FVoxelParameterOverridesRuntime> Create(const FVoxelParameterOverridesOwnerPtr& OverridesOwner);
	UE_NONCOPYABLE(FVoxelParameterOverridesRuntime);

	VOXEL_COUNT_INSTANCES();

public:
	FVoxelFutureValue FindParameter(
		const FVoxelPinType& Type,
		const FVoxelParameterPath& Path,
		const FVoxelQuery& Query) const;

private:
	const FVoxelParameterOverridesOwnerPtr OverridesOwner;

	mutable FVoxelCriticalSection CriticalSection;

	struct FParameterValue
	{
		FVoxelRuntimePinValue Value;
		TSharedPtr<FVoxelDependency> Dependency;
	};
	TVoxelMap<FVoxelParameterPath, FParameterValue> PathToValue_RequiresLock;

	explicit FVoxelParameterOverridesRuntime(const FVoxelParameterOverridesOwnerPtr& OverridesOwner)
		: OverridesOwner(OverridesOwner)
	{
	}

private:
	FVoxelRuntimePinValue FindParameter_GameThread(
		const FVoxelPinType& Type,
		const FVoxelParameterPath& Path,
		const FVoxelQuery& Query);

private:
	void Update_GameThread();
	void Update_GameThread_RequiresLock();

	friend class FVoxelParameterOverridesRuntimesManager;
};