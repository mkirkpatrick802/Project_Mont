// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphParametersView.h"
#include "VoxelGraph.h"
#include "VoxelParameterView.h"
#include "VoxelGraphParametersViewContext.h"

void FVoxelGraphParametersViewBase::Initialize(
	FVoxelGraphParametersViewContext& Context,
	const FVoxelParameterPath& BasePath,
	const UVoxelGraph& Graph)
{
	VOXEL_FUNCTION_COUNTER();

	for (const FGuid& Guid : Graph.GetParameters())
	{
		const FVoxelParameter& Parameter = Graph.FindParameterChecked(Guid);
		if (!ensure(!NameToParameterView.Contains(Parameter.Name)))
		{
			continue;
		}

		const TSharedRef<FVoxelParameterView> ParameterView = MakeVoxelShared<FVoxelParameterView>(
			Context,
			BasePath.MakeChild(Guid),
			Parameter);

		ChildrenRefs.Add(ParameterView);
		Children.Add(&ParameterView.Get());
		GuidToParameterView.Add_CheckNew(Guid, &ParameterView.Get());
		NameToParameterView.Add_CheckNew(Parameter.Name, &ParameterView.Get());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelArray<TVoxelArray<FVoxelParameterView*>> FVoxelGraphParametersViewBase::GetCommonChildren(const TConstVoxelArrayView<const FVoxelGraphParametersViewBase*> ParametersViews)
{
	VOXEL_FUNCTION_COUNTER();

	if (ParametersViews.Num() == 0)
	{
		return {};
	}

	bool bGuidsSet = false;
	TVoxelSet<FGuid> Guids;
	for (const FVoxelGraphParametersViewBase* ParametersView : ParametersViews)
	{
		const TConstVoxelArrayView<FVoxelParameterView*> Children = ParametersView->GetChildren();

		TVoxelSet<FGuid> NewGuids;
		NewGuids.Reserve(Children.Num());

		for (const FVoxelParameterView* Child : Children)
		{
			ensure(!NewGuids.Contains(Child->GetGuid()));
			NewGuids.Add(Child->GetGuid());
		}

		if (bGuidsSet)
		{
			Guids = Guids.Intersect(NewGuids);
		}
		else
		{
			bGuidsSet = true;
			Guids = MoveTemp(NewGuids);
		}
	}

	const TVoxelArray<FGuid> GuidArray = Guids.Array();

	TVoxelArray<TVoxelArray<FVoxelParameterView*>> Result;
	Result.SetNum(GuidArray.Num());

	for (const FVoxelGraphParametersViewBase* ParametersView : ParametersViews)
	{
		for (int32 Index = 0; Index < GuidArray.Num(); Index++)
		{
			const FGuid Guid = GuidArray[Index];

			FVoxelParameterView* ChildParameterView = ParametersView->FindByGuid(Guid);
			if (!ensure(ChildParameterView))
			{
				return {};
			}

			Result[Index].Add(ChildParameterView);
		}
	}

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelGraphParametersView::FVoxelGraphParametersView(const UVoxelGraph& Graph)
	: Context(MakeVoxelShared<FVoxelGraphParametersViewContext>())
{
	for (const UVoxelGraph* BaseGraph : Graph.GetBaseGraphs())
	{
		Context->AddGraphOverride(FVoxelParameterPath::MakeEmpty(), *BaseGraph);
	}

	Initialize(
		GetContext(),
		FVoxelParameterPath::MakeEmpty(),
		Graph);
}

FVoxelParameterView* FVoxelGraphParametersView::FindChild(const FVoxelParameterPath& Path) const
{
	if (!ensureVoxelSlow(Path.Guids.Num() > 0))
	{
		return nullptr;
	}

	FVoxelParameterView* ParameterView = FindByGuid(Path.Guids[0]);
	if (!ParameterView)
	{
		return nullptr;
	}

	for (int32 Index = 1; Index < Path.Guids.Num(); Index++)
	{
		ParameterView = ParameterView->FindByGuid(Path.Guids[Index]);

		if (!ParameterView)
		{
			return nullptr;
		}
	}
	checkVoxelSlow(ParameterView->Path == Path);

	return ParameterView;
}