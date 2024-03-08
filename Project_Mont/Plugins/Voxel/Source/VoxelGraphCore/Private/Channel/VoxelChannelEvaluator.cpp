// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Channel/VoxelChannelEvaluator.h"
#include "Channel/VoxelRuntimeChannel.h"
#include "VoxelSurface.h"
#include "VoxelFutureValueState.h"
#include "VoxelDependencyTracker.h"
#include "VoxelPositionQueryParameter.h"
#include "Nodes/VoxelNode_QueryChannel.h"
#include "Utilities/VoxelBufferGatherer.h"
#include "Utilities/VoxelBufferScatterer.h"
#include "Nodes/VoxelNode_GetPreviousChannelValue.h"

FVoxelChannelEvaluator::FVoxelChannelEvaluator(
	const TSharedRef<const FVoxelRuntimeChannel>& Channel,
	const FVoxelQuery& Query)
	: Channel(Channel)
	, Definition(Channel->Definition)
	, Query(Query)
{
}

FVoxelFutureValue FVoxelChannelEvaluator::Compute()
{
	const FVoxelPositionQueryParameter* PositionQueryParameter = Query.GetParameters().Find<FVoxelPositionQueryParameter>();
	if (!ensure(PositionQueryParameter))
	{
		return Definition.DefaultValue;
	}
	const FVoxelVectorBuffer Positions = PositionQueryParameter->GetPositions();

	VOXEL_FUNCTION_COUNTER_NUM(Positions.Num(), 1);

	const FVoxelBox Bounds = PositionQueryParameter->GetBounds();

	if (const FVoxelMinExactDistanceQueryParameter* MinExactDistanceQueryParameter = Query.GetParameters().Find<FVoxelMinExactDistanceQueryParameter>())
	{
		MinExactDistance = MinExactDistanceQueryParameter->MinExactDistance;
	}

	FVoxelBrushPriority Priority = FVoxelBrushPriority::Max();
	if (const FVoxelBrushPriorityQueryParameter* BrushPriorityQueryParameter = Query.GetParameters().Find<FVoxelBrushPriorityQueryParameter>())
	{
		if (const FVoxelBrushPriority* PriorityPtr = BrushPriorityQueryParameter->ChannelToPriority.Find(Definition.Name))
		{
			Priority = *PriorityPtr;
		}
	}

	GatherBrushes(Bounds.Extend(MinExactDistance), Priority);

	if (Brushes.Num() == 0)
	{
		return Definition.DefaultValue;
	}

	Brushes.Sort([](const TSharedPtr<const FVoxelBrush>& BrushA, const TSharedPtr<const FVoxelBrush>& BrushB)
	{
		// Process highest priority last
		return BrushA->Priority < BrushB->Priority;
	});

	FutureValueStateImpl = MakeVoxelShared<FVoxelFutureValueStateImpl>(Definition.Type, STATIC_FNAME("FVoxelChannelEvaluator"));

	ComputeNext(true, Definition.DefaultValue, 0);

	return FVoxelFutureValue(FutureValueStateImpl.ToSharedRef());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChannelEvaluator::GatherBrushes(
	const FVoxelBox& Bounds,
	const FVoxelBrushPriority Priority)
{
	VOXEL_FUNCTION_COUNTER();

	Query.GetDependencyTracker().AddDependency(
		Channel->Dependency,
		Bounds,
		Priority.Raw);

	VOXEL_SCOPE_LOCK(Channel->CriticalSection);

	for (const auto& It : Channel->RuntimeBrushes_RequiresLock)
	{
		const FVoxelRuntimeChannel::FRuntimeBrush& Brush = *It.Value;

		if (Brush.Priority >= Priority ||
			// Can happen due to race condition in FVoxelRuntimeChannel::AddBrush
			!ensureVoxelSlow(Brush.RuntimeBounds_RequiresLock.IsSet()) ||
			!Brush.RuntimeBounds_RequiresLock.GetValue().Intersect(Bounds))
		{
			continue;
		}

		Brushes.Add(Brush.Brush);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelChannelEvaluator::ComputeNext(
	const bool bIsPreviousDefault,
	const FVoxelRuntimePinValue& PreviousValue,
	const int32 BrushIndex) const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(PreviousValue.CanBeCastedTo(Definition.Type));

	if (BrushIndex == Brushes.Num())
	{
		FutureValueStateImpl->SetValue(PreviousValue);
		return;
	}

	const FVoxelFutureValue FutureValue = INLINE_LAMBDA
	{
		const TSharedRef<const FVoxelBrush> Brush = Brushes[BrushIndex].ToSharedRef();
		const TVoxelUniquePtr<FVoxelTaskTagScope> Scope = FVoxelTaskTagScope::Create(Brush->DebugNameWithBrushPrefix);

		return GetBrushValue(
			bIsPreviousDefault,
			PreviousValue,
			Brush);
	};

	MakeVoxelTask()
	.Dependency(FutureValue)
	.Execute(MakeWeakPtrLambda(this, [=]
	{
		ComputeNext(
			false,
			FutureValue.GetValue_CheckCompleted(),
			BrushIndex + 1);
	}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelChannelEvaluator::GetBrushValue(
	const bool bIsPreviousDefault,
	const FVoxelRuntimePinValue& PreviousValue,
	const TSharedRef<const FVoxelBrush>& Brush) const
{
	VOXEL_FUNCTION_COUNTER();

	if (PreviousValue.IsBuffer())
	{
		const FVoxelFutureValue FutureValue = TryGetBrushValue_Buffer(
			bIsPreviousDefault,
			PreviousValue.GetSharedStruct<FVoxelBuffer>(),
			Brush);

		if (FutureValue.IsValid())
		{
			return FutureValue;
		}
	}
	if (PreviousValue.Is<FVoxelSurface>())
	{
		const FVoxelFutureValue FutureValue = TryGetBrushValue_Surface(
			bIsPreviousDefault,
			PreviousValue.GetSharedStruct<FVoxelSurface>(),
			Brush);

		if (FutureValue.IsValid())
		{
			return FutureValue;
		}
	}

	// No filtering
	return
		MakeVoxelTask()
		.Execute(Definition.Type, MakeWeakPtrLambda(this, [=]
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelPreviousChannelValueQueryParameter>().Initialize(bIsPreviousDefault, PreviousValue);
			return Brush->Compute(Query.MakeNewQuery(Parameters));
		}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelChannelEvaluator::TryFilter(
	const TSharedRef<const FVoxelBrush>& Brush,
	FVoxelVectorBuffer& OutPositions,
	FVoxelInt32Buffer& OutIndices,
	FVoxelVectorBuffer& OutFilteredPositions) const
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<const FVoxelPositionQueryParameter> PositionQueryParameter = Query.GetParameters().FindShared<FVoxelPositionQueryParameter>();
	if (!ensure(PositionQueryParameter))
	{
		return false;
	}

	OutPositions = PositionQueryParameter->GetPositions();

	const FVoxelTransformRef LocalToQuery = Brush->LocalToWorld * Query.GetLocalToWorld().Inverse();
	const FVoxelBox BrushBounds = Brush->LocalBounds.TransformBy(LocalToQuery.Get(Query)).Extend(MinExactDistance);

	if (!PositionQueryParameter->TryFilter(BrushBounds, OutIndices, OutFilteredPositions))
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelChannelEvaluator::TryGetBrushValue_Buffer(
	const bool bIsPreviousDefault,
	const TSharedRef<const FVoxelBuffer>& PreviousBuffer,
	const TSharedRef<const FVoxelBrush>& Brush) const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelVectorBuffer Positions;
	FVoxelInt32Buffer Indices;
	FVoxelVectorBuffer FilteredPositions;
	if (!TryFilter(
		Brush,
		Positions,
		Indices,
		FilteredPositions))
	{
		return {};
	}

	if (Indices.Num() == 0)
	{
		// Even when bounds intersect, all positions might be outside of the brush bounds
		return FVoxelRuntimePinValue::Make(PreviousBuffer, Definition.Type);
	}

	const TSharedRef<const FVoxelBuffer> FilteredPreviousBuffer = FVoxelBufferGatherer::Gather(*PreviousBuffer, Indices);

	const TVoxelFutureValue<FVoxelBuffer> FilteredFutureBuffer =
		MakeVoxelTask()
		.Execute<FVoxelBuffer>(Definition.Type, MakeWeakPtrLambda(this, [=]
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelPositionQueryParameter>().Initialize(FilteredPositions);
			Parameters->Add<FVoxelPreviousChannelValueQueryParameter>().Initialize(
				bIsPreviousDefault,
				FVoxelRuntimePinValue::Make(FilteredPreviousBuffer, Definition.Type));

			return TVoxelFutureValue<FVoxelBuffer>(Brush->Compute(Query.MakeNewQuery(Parameters)));
		}));

	return
		MakeVoxelTask()
		.Dependency(FilteredFutureBuffer)
		.Execute(Definition.Type, MakeWeakPtrLambda(this, [=]
		{
			const TSharedRef<const FVoxelBuffer> NewBuffer = FVoxelBufferScatterer::Scatter(
				*PreviousBuffer,
				Positions.Num(),
				FilteredFutureBuffer.Get_CheckCompleted(),
				Indices);

			return FVoxelRuntimePinValue::Make(NewBuffer, Definition.Type);
		}));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelChannelEvaluator::TryGetBrushValue_Surface(
	const bool bIsPreviousDefault,
	const TSharedRef<const FVoxelSurface>& PreviousSurface,
	const TSharedRef<const FVoxelBrush>& Brush) const
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelVectorBuffer Positions;
	FVoxelInt32Buffer Indices;
	FVoxelVectorBuffer FilteredPositions;
	if (!TryFilter(
		Brush,
		Positions,
		Indices,
		FilteredPositions))
	{
		return {};
	}

	if (Indices.Num() == 0)
	{
		// Even when bounds intersect, all positions might be outside of the brush bounds
		return FVoxelRuntimePinValue::Make(PreviousSurface);
	}

	const TSharedRef<FVoxelSurface> FilteredPreviousSurface = PreviousSurface->MakeSharedCopy();
	for (const auto& It : PreviousSurface->GetNameToAttribute())
	{
		VOXEL_SCOPE_COUNTER_FNAME(It.Key);
		const TSharedRef<const FVoxelBuffer> FilteredAttribute = FVoxelBufferGatherer::Gather(*It.Value, Indices);
		FilteredPreviousSurface->Set(It.Key, FilteredAttribute);
	}

	const TVoxelFutureValue<FVoxelSurface> FilteredFutureSurface =
		MakeVoxelTask()
		.Execute<FVoxelSurface>(MakeWeakPtrLambda(this, [=]
		{
			const TSharedRef<FVoxelQueryParameters> Parameters = Query.CloneParameters();
			Parameters->Add<FVoxelPositionQueryParameter>().Initialize(FilteredPositions);
			Parameters->Add<FVoxelPreviousChannelValueQueryParameter>().Initialize(
				bIsPreviousDefault,
				FVoxelRuntimePinValue::Make(FilteredPreviousSurface));

			return TVoxelFutureValue<FVoxelSurface>(Brush->Compute(Query.MakeNewQuery(Parameters)));
		}));

	return
		MakeVoxelTask()
		.Dependency(FilteredFutureSurface)
		.Execute<FVoxelSurface>(MakeWeakPtrLambda(this, [=]
		{
			const FVoxelSurface& FilteredSurface = FilteredFutureSurface.Get_CheckCompleted();

			const TSharedRef<FVoxelSurface> NewSurface = FilteredSurface.MakeSharedCopy();

			TVoxelSet<FName> Attributes;
			Attributes.Reserve(PreviousSurface->GetNameToAttribute().Num() + FilteredSurface.GetNameToAttribute().Num());
			for (const auto& It : PreviousSurface->GetNameToAttribute())
			{
				Attributes.Add(It.Key);
			}
			for (const auto& It : FilteredSurface.GetNameToAttribute())
			{
				Attributes.Add(It.Key);
			}

			for (const FName Attribute : Attributes)
			{
				VOXEL_SCOPE_COUNTER_FNAME(Attribute);

				TSharedPtr<const FVoxelBuffer> BaseBuffer = PreviousSurface->GetNameToAttribute().FindRef(Attribute);
				if (!BaseBuffer)
				{
					const FVoxelPinType InnerType = FilteredSurface.GetNameToAttribute().FindChecked(Attribute)->GetInnerType();
					BaseBuffer = FVoxelSurfaceAttributes::MakeDefault(InnerType, Attribute);
				}

				const TSharedPtr<const FVoxelBuffer> FilteredBuffer = FilteredSurface.GetNameToAttribute().FindRef(Attribute);
				if (!FilteredBuffer)
				{
					// Assume the brush doesn't want to edit this attribute
					NewSurface->Set(Attribute, BaseBuffer.ToSharedRef());
					continue;
				}

				const TSharedRef<const FVoxelBuffer> NewBuffer = FVoxelBufferScatterer::Scatter(
					*BaseBuffer,
					Positions.Num(),
					*FilteredBuffer,
					Indices);

				NewSurface->Set(Attribute, NewBuffer);
			}
			return NewSurface.ToSharedPtr();
		}));
}