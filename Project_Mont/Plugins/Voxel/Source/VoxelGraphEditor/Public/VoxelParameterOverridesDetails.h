// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelParameterPath.h"
#include "VoxelParameterOverridesOwner.h"

class FVoxelParameterView;
class FVoxelCategoryBuilder;
class FVoxelParameterDetails;
class FVoxelGraphParametersView;
struct FVoxelParameterOverrides;

class VOXELGRAPHEDITOR_API FVoxelParameterOverridesDetails
	: public FVoxelTicker
	, public TSharedFromThis<FVoxelParameterOverridesDetails>
{
public:
	static TSharedPtr<FVoxelParameterOverridesDetails> Create(
		IDetailLayoutBuilder& DetailLayout,
		const TVoxelArray<FVoxelParameterOverridesOwnerPtr>& Owners,
		const FSimpleDelegate& RefreshDelegate,
		TOptional<FGuid> ParameterDefaultValueGuid = {});

	template<typename ObjectType, typename = std::enable_if_t<TIsDerivedFrom<ObjectType, IVoxelParameterOverridesObjectOwner>::Value>>
	static TSharedPtr<FVoxelParameterOverridesDetails> Create(
		IDetailLayoutBuilder& DetailLayout,
		const TVoxelArray<ObjectType*>& Owners,
		const FSimpleDelegate& RefreshDelegate,
		const TOptional<FGuid> ParameterDefaultValueGuid = {})
	{
		return Create(
			DetailLayout,
			TVoxelArray<FVoxelParameterOverridesOwnerPtr>(Owners),
			RefreshDelegate,
			ParameterDefaultValueGuid);
	}

	static TSharedPtr<FVoxelParameterOverridesDetails> Create(
		FVoxelDetailInterface DetailInterface,
		const TVoxelArray<FVoxelParameterOverridesOwnerPtr>& Owners,
		const FSimpleDelegate& RefreshDelegate);

	VOXEL_COUNT_INSTANCES();

	//~ Begin FVoxelTicker Interface
	virtual void Tick() override;
	//~ End FVoxelTicker Interface

public:
	TVoxelArray<IVoxelParameterOverridesOwner*> GetOwners() const;

	void ForceRefresh() const
	{
		ensure(RefreshDelegate.ExecuteIfBound());
	}

	void GenerateView(
		const TVoxelArray<FVoxelParameterView*>& ParameterViews,
		const FVoxelDetailInterface& DetailInterface);

	void AddOrphans(
		FVoxelCategoryBuilder& CategoryBuilder,
		const FVoxelParameterPath& BasePath);

private:
	const TVoxelArray<FVoxelParameterOverridesOwnerPtr> Owners;
	const FSimpleDelegate RefreshDelegate;
	const TOptional<FGuid> ParameterDefaultValueGuid;

	TVoxelArray<TSharedRef<FVoxelGraphParametersView>> GraphParametersViewRefs;
	TVoxelArray<FVoxelGraphParametersView*> GraphParametersViews;
	TVoxelArray<TVoxelArray<FVoxelParameterView*>> RootParameterViews;
	TVoxelMap<FVoxelParameterPath, TSharedPtr<FVoxelParameterDetails>> PathToParameterDetails;

private:
	FVoxelParameterOverridesDetails(
		const TVoxelArray<FVoxelParameterOverridesOwnerPtr>& Owners,
		const FSimpleDelegate& RefreshDelegate,
		const TOptional<FGuid> ParameterDefaultValueGuid)
		: Owners(Owners)
		, RefreshDelegate(RefreshDelegate)
		, ParameterDefaultValueGuid(ParameterDefaultValueGuid)
	{
	}

	void Initialize();
	void Initialize(FVoxelCategoryBuilder& CategoryBuilder);
};