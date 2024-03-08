// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelParameterOverridesDetails.h"
#include "Customizations/VoxelParameterDetails.h"
#include "VoxelGraph.h"
#include "VoxelParameterView.h"
#include "VoxelCategoryBuilder.h"
#include "VoxelGraphParametersView.h"
#include "VoxelParameterOverridesOwner.h"
#include "VoxelGraphParametersViewContext.h"

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelParameterOverridesDetails);

TSharedPtr<FVoxelParameterOverridesDetails> FVoxelParameterOverridesDetails::Create(
	IDetailLayoutBuilder& DetailLayout,
	const TVoxelArray<FVoxelParameterOverridesOwnerPtr>& Owners,
	const FSimpleDelegate& RefreshDelegate,
	const TOptional<FGuid> ParameterDefaultValueGuid)
{
	ensure(RefreshDelegate.IsBound());

	const TSharedRef<FVoxelParameterOverridesDetails> Result = MakeVoxelShareable(new (GVoxelMemory) FVoxelParameterOverridesDetails(
		Owners,
		RefreshDelegate,
		ParameterDefaultValueGuid));

	Result->Initialize();

	if (!ensure(Result->GraphParametersViewRefs.Num() > 0))
	{
		return nullptr;
	}

	FVoxelCategoryBuilder CategoryBuilder({});
	Result->Initialize(CategoryBuilder);

	if (ParameterDefaultValueGuid.IsSet())
	{
		// Skip categories
		CategoryBuilder.ApplyFlat(DetailLayout.EditCategory("Default Value"));
	}
	else
	{
		CategoryBuilder.Apply(DetailLayout);
	}

	return Result;
}

TSharedPtr<FVoxelParameterOverridesDetails> FVoxelParameterOverridesDetails::Create(
	const FVoxelDetailInterface DetailInterface,
	const TVoxelArray<FVoxelParameterOverridesOwnerPtr>& Owners,
	const FSimpleDelegate& RefreshDelegate)
{
	ensure(RefreshDelegate.IsBound());

	const TSharedRef<FVoxelParameterOverridesDetails> Result = MakeVoxelShareable(new (GVoxelMemory) FVoxelParameterOverridesDetails(
		Owners,
		RefreshDelegate,
		{}));

	Result->Initialize();

	if (!ensure(Result->GraphParametersViewRefs.Num() > 0))
	{
		return nullptr;
	}

	FVoxelCategoryBuilder CategoryBuilder({});
	Result->Initialize(CategoryBuilder);
	CategoryBuilder.Apply(DetailInterface);

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterOverridesDetails::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	for (auto It = PathToParameterDetails.CreateIterator(); It; ++It)
	{
		const TSharedPtr<FVoxelParameterDetails> ParameterDetails = It.Value();
		if (!ParameterDetails->IsValid())
		{
			// Parent ParameterDetails was collapsed
			It.RemoveCurrent();
			continue;
		}

		ParameterDetails->Tick();

		if (!ParameterDetails->IsValid())
		{
			// Remove now to avoid ticking StructWrapper with an invalid handle
			It.RemoveCurrent();
		}
	}

	// If root parameters changed, trigger a full refresh
	if (RootParameterViews != FVoxelGraphParametersView::GetCommonChildren(GraphParametersViews))
	{
		ForceRefresh();
		return;
	}

	// Check for type changes
	for (const auto& It : PathToParameterDetails)
	{
		const TSharedPtr<FVoxelParameterDetails> ParameterDetails = It.Value;
		if (ParameterDetails->IsOrphan())
		{
			continue;
		}

		const FVoxelParameterView* ParameterView = ParameterDetails->ParameterViews[0];
		if (!ensure(ParameterView))
		{
			return;
		}

		if (ParameterDetails->bIsInlineGraph != ParameterView->IsInlineGraph() ||
			ParameterDetails->RowExposedType != ParameterView->GetType().GetExposedType())
		{
			// Don't bother doing granular updates on type change
			ForceRefresh();
			return;
		}
	}

	// Check for layout changes
	TSet<FVoxelParameterPath> PathsToRebuild;
	for (const auto& It : PathToParameterDetails)
	{
		if (It.Value->ShouldRebuildChildren())
		{
			PathsToRebuild.Add(It.Value->Path);
		}
	}

	// Process children first
	PathsToRebuild.Sort([](const FVoxelParameterPath& A, const FVoxelParameterPath& B)
	{
		return A.Num() > B.Num();
	});

	// Remove children whose parents are going to be rebuilt
	for (auto It = PathsToRebuild.CreateIterator(); It; ++It)
	{
		if (It->Num() == 1)
		{
			continue;
		}
		ensure(It->Num() > 1);

		for (FVoxelParameterPath Path = It->GetParent(); Path.Num() > 1; Path = Path.GetParent())
		{
			if (PathsToRebuild.Contains(Path))
			{
				It.RemoveCurrent();
				break;
			}
		}
	}

	// Rebuild
	for (const FVoxelParameterPath& Path : PathsToRebuild)
	{
		const TSharedPtr<FVoxelParameterDetails> ParameterDetails = PathToParameterDetails.FindRef(Path);
		if (!ensure(ParameterDetails))
		{
			continue;
		}

		ParameterDetails->RebuildChildren();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelArray<IVoxelParameterOverridesOwner*> FVoxelParameterOverridesDetails::GetOwners() const
{
	TVoxelArray<IVoxelParameterOverridesOwner*> Result;
	for (const FVoxelParameterOverridesOwnerPtr& WeakOwner : Owners)
	{
		if (IVoxelParameterOverridesOwner* Owner = WeakOwner.Get())
		{
			Result.Add(Owner);
		}
	}
	return Result;
}

void FVoxelParameterOverridesDetails::GenerateView(
	const TVoxelArray<FVoxelParameterView*>& ParameterViews,
	const FVoxelDetailInterface& DetailInterface)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(ParameterViews.Num() > 0))
	{
		return;
	}

	const FVoxelParameterPath Path = ParameterViews[0]->Path;
	for (const FVoxelParameterView* ParameterView : ParameterViews)
	{
		ensure(Path == ParameterView->Path);
	}

	if (ParameterDefaultValueGuid.IsSet() &&
		!Path.StartsWith(FVoxelParameterPath::Make(ParameterDefaultValueGuid.GetValue())))
	{
		return;
	}

	const TSharedRef<FVoxelParameterDetails> ParameterDetails = MakeVoxelShared<FVoxelParameterDetails>(
		*this,
		Path,
		ParameterViews);
	PathToParameterDetails.Add_EnsureNew(Path, ParameterDetails);

	ParameterDetails->MakeRow(DetailInterface);
}

void FVoxelParameterOverridesDetails::AddOrphans(
	FVoxelCategoryBuilder& CategoryBuilder,
	const FVoxelParameterPath& BasePath)
{
	VOXEL_FUNCTION_COUNTER();

	if (ParameterDefaultValueGuid.IsSet())
	{
		// Never add orphans if we have a whitelist
		return;
	}

	bool bCommonPathSet = false;
	TVoxelSet<FVoxelParameterPath> CommonPaths;

	for (const IVoxelParameterOverridesOwner* Owner : GetOwners())
	{
		TVoxelSet<FVoxelParameterPath> Paths;
		for (const auto& It : Owner->GetPathToValueOverride())
		{
			if (It.Key.GetParent() != BasePath)
			{
				continue;
			}

			Paths.Add(It.Key);
		}

		if (bCommonPathSet)
		{
			CommonPaths = CommonPaths.Intersect(Paths);
		}
		else
		{
			bCommonPathSet = true;
			CommonPaths = MoveTemp(Paths);
		}
	}

	for (const FVoxelParameterPath& Path : CommonPaths)
	{
		ensure(Path.GetParent() == BasePath);

		bool bIsAnyValid = false;
		for (FVoxelGraphParametersView* GraphParametersView : GraphParametersViews)
		{
			if (GraphParametersView->FindChild(Path))
			{
				bIsAnyValid = true;
				break;
			}
		}

		if (bIsAnyValid)
		{
			// Not orphan for at least one of the views, skip
			continue;
		}

		FName Name;
		FString Category;
		FVoxelPinValue Value;
		bool bHasSingleValue = true;
		{
			TVoxelArray<FVoxelParameterValueOverride> ValueOverrides;
			for (const IVoxelParameterOverridesOwner* Owner : GetOwners())
			{
				ValueOverrides.Add(Owner->GetPathToValueOverride().FindChecked(Path));
			}

			if (!ensure(ValueOverrides.Num() > 0))
			{
				continue;
			}

			Name = ValueOverrides[0].CachedName;
			Category = ValueOverrides[0].CachedCategory;
			Value = ValueOverrides[0].Value;
			for (const FVoxelParameterValueOverride& ValueOverride : ValueOverrides)
			{
				if (Name != ValueOverride.CachedName)
				{
					Name = "Multiple Values";
				}
				if (Category != ValueOverride.CachedCategory)
				{
					Category.Reset();
				}
				if (Value != ValueOverride.Value)
				{
					bHasSingleValue = false;
				}
			}
		}

		const TSharedRef<FVoxelParameterDetails> ParameterDetails = MakeVoxelShared<FVoxelParameterDetails>(
			*this,
			Path,
			TVoxelArray<FVoxelParameterView*>());

		ParameterDetails->OrphanName = Name;
		ParameterDetails->OrphanExposedType = Value.GetType();
		ParameterDetails->InitializeOrphan(Value, bHasSingleValue);
		PathToParameterDetails.Add_EnsureNew(Path, ParameterDetails);

		CategoryBuilder.AddProperty(
			Category,
			MakeWeakPtrLambda(this, [ParameterDetails](const FVoxelDetailInterface& DetailInterface)
			{
				ParameterDetails->MakeRow(DetailInterface);
			}));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelParameterOverridesDetails::Initialize()
{
	VOXEL_FUNCTION_COUNTER();

	for (IVoxelParameterOverridesOwner* Owner : GetOwners())
	{
		Owner->FixupParameterOverrides();
		Owner->OnParameterLayoutChanged.Add(MakeWeakPtrDelegate(this, [this]
		{
			ForceRefresh();
		}));

		const UVoxelGraph* Graph = Owner->GetGraph();
		if (!Graph)
		{
			continue;
		}
		const TSharedRef<FVoxelGraphParametersView> ParametersView = Owner->GetParametersView_ValidGraph();

		// We want GetValue to show the override value even if it's disabled
		ParametersView->GetContext().ForceEnableOverrides();

		GraphParametersViewRefs.Add(ParametersView);
		GraphParametersViews.Add(&ParametersView.Get());
	}

	RootParameterViews = FVoxelGraphParametersView::GetCommonChildren(GraphParametersViews);
}

void FVoxelParameterOverridesDetails::Initialize(FVoxelCategoryBuilder& CategoryBuilder)
{
	VOXEL_FUNCTION_COUNTER();

	for (const TVoxelArray<FVoxelParameterView*>& ChildParameterViews : RootParameterViews)
	{
		const FString Category = ChildParameterViews[0]->GetParameter().Category;
		for (const FVoxelParameterView* ChildParameterView : ChildParameterViews)
		{
			ensure(ChildParameterView->GetParameter().Category == Category);
		}

		CategoryBuilder.AddProperty(
			Category,
			MakeWeakPtrLambda(this, [=](const FVoxelDetailInterface& DetailInterface)
			{
				GenerateView(ChildParameterViews, DetailInterface);
			}));
	}

	AddOrphans(CategoryBuilder, FVoxelParameterPath::MakeEmpty());
}