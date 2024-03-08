// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelParameterView.h"
#include "VoxelGraph.h"
#include "VoxelGraphParametersView.h"
#include "VoxelGraphParametersViewContext.h"

FVoxelParameterView::FVoxelParameterView(
	FVoxelGraphParametersViewContext& Context,
	const FVoxelParameterPath& Path,
	const FVoxelParameter& Parameter)
	: Context(Context)
	, Path(Path)
	, Parameter(Parameter)
{
}

FVoxelPinType FVoxelParameterView::GetType() const
{
	return IsInlineGraph()
		? FVoxelPinType::Make<UVoxelGraph>()
		: Parameter.Type;
}

FVoxelPinValue FVoxelParameterView::GetValue() const
{
	VOXEL_FUNCTION_COUNTER();

	const FVoxelPinType ExposedType = GetType().GetExposedType();

	const FVoxelParameterValueOverride* ValueOverride = Context.FindValueOverride(Path);
	if (!ValueOverride)
	{
		return FVoxelPinValue(ExposedType);
	}

	ensure(ValueOverride->Value.CanBeCastedTo(ExposedType));
	return ValueOverride->Value;
}

TConstVoxelArrayView<FVoxelParameterView*> FVoxelParameterView::GetChildren() const
{
	const FVoxelGraphParametersViewBase* InlineGraphParametersView = GetInlineGraphParametersView();
	if (!InlineGraphParametersView)
	{
		return {};
	}
	return InlineGraphParametersView->GetChildren();
}

FVoxelParameterView* FVoxelParameterView::FindByGuid(const FGuid& Guid) const
{
	const FVoxelGraphParametersViewBase* InlineGraphParametersView = GetInlineGraphParametersView();
	if (!InlineGraphParametersView)
	{
		return nullptr;
	}
	return InlineGraphParametersView->FindByGuid(Guid);
}

FVoxelParameterView* FVoxelParameterView::FindByName(const FName Name) const
{
	const FVoxelGraphParametersViewBase* InlineGraphParametersView = GetInlineGraphParametersView();
	if (!InlineGraphParametersView)
	{
		return nullptr;
	}
	return InlineGraphParametersView->FindByName(Name);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelArray<TVoxelArray<FVoxelParameterView*>> FVoxelParameterView::GetCommonChildren(const TConstVoxelArrayView<FVoxelParameterView*> ParameterViews)
{
	TVoxelArray<const FVoxelGraphParametersViewBase*> InlineGraphParametersViews;
	for (const auto ParameterView : ParameterViews)
	{
		const FVoxelGraphParametersViewBase* InlineGraphParametersView = ParameterView->GetInlineGraphParametersView();
		if (!InlineGraphParametersView)
		{
			return {};
		}

		InlineGraphParametersViews.Add(InlineGraphParametersView);
	}
	return FVoxelGraphParametersViewBase::GetCommonChildren(InlineGraphParametersViews);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelGraphParametersViewBase* FVoxelParameterView::GetInlineGraphParametersView() const
{
	if (!IsInlineGraph())
	{
		return nullptr;
	}

	const FVoxelPinValue Value = GetValue();
	if (!ensure(Value.Is<UVoxelGraph>()))
	{
		return nullptr;
	}

	const UVoxelGraph* Graph = Value.Get<UVoxelGraph>();
	if (!Graph)
	{
		return nullptr;
	}

	return &Context.FindOrAddParametersView(Path, *Graph);
}