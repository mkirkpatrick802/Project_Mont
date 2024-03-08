// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_GetSurfaceAttributes.h"

FVoxelNode_GetSurfaceAttributes::FVoxelNode_GetSurfaceAttributes()
{
	FixupPins();
}

void FVoxelNode_GetSurfaceAttributes::PostSerialize()
{
	FixupPins();

	Super::PostSerialize();
}

FVoxelComputeValue FVoxelNode_GetSurfaceAttributes::CompileCompute(const FName PinName) const
{
	const FVoxelNodeRuntime::FPinData& PinData = GetNodeRuntime().GetPinData(PinName);

	return [this, PinName, PinType = PinData.Type](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));
		FVoxelQueryScope Scope(Query);

		const FVoxelQuery NewQuery = INLINE_LAMBDA
		{
			const FVoxelSurfaceQueryParameter* SurfaceQueryParameter = Query.GetParameters().Find<FVoxelSurfaceQueryParameter>();
			if (SurfaceQueryParameter &&
				SurfaceQueryParameter->ShouldCompute(PinName))
			{
				return Query;
			}

			const TSharedRef<FVoxelSurfaceQueryParameter> NewSurfaceQueryParameter = MakeVoxelShared<FVoxelSurfaceQueryParameter>();
			if (SurfaceQueryParameter)
			{
				NewSurfaceQueryParameter->AttributesToCompute = SurfaceQueryParameter->AttributesToCompute;
			}
			NewSurfaceQueryParameter->AttributesToCompute.Add_CheckNew(PinName);

			return Query.MakeNewQuery(NewSurfaceQueryParameter);
		};

		const TValue<FVoxelSurface> FutureSurface = GetNodeRuntime().Get(SurfacePin, NewQuery);

		return
			MakeVoxelTask()
			.Dependency(FutureSurface)
			.Execute(PinType, [=]() -> FVoxelFutureValue
			{
				FVoxelQueryScope LocalScope(Query);
				const FVoxelSurface& Surface = FutureSurface.Get_CheckCompleted();

				// Don't error if we're the default value
				if (Surface.NameToAttribute.Num() > 0 &&
					!Surface.AttributesToCompute.Contains(PinName))
				{
					ensure(!Surface.NameToAttribute.Contains(PinName));

					VOXEL_MESSAGE(Error,
						"{0}: trying to query {1}, but it's not part of the attributes to compute. "
						"Use Add Surface Attribute Dependency in your main graph with Dependency = {1} and Attribute = {2}",
						this,
						PinName,
						Surface.AttributesToCompute.Array());
				}

				const TSharedPtr<const FVoxelBuffer> Attribute = Surface.NameToAttribute.FindRef(PinName);
				if (!Attribute)
				{
					// Return default value, don't error out
					checkVoxelSlow(PinType.IsBuffer());
					const TSharedRef<const FVoxelBuffer> DefaultBuffer = FVoxelSurfaceAttributes::MakeDefault(PinType.GetInnerType(), PinName);
					return FVoxelRuntimePinValue::Make(DefaultBuffer, PinType);
				}
				ensure(Surface.AttributesToCompute.Contains(PinName));

				if (!Attribute->GetBufferType().CanBeCastedTo(PinType))
				{
					VOXEL_MESSAGE(Error, "{0}: attribute type is {1}, but was queried as {2}",
						this,
						Attribute->GetBufferType().ToString(),
						PinType.ToString());
					return {};
				}

				return FVoxelRuntimePinValue::Make(Attribute.ToSharedRef(), PinType);
			});
	};
}

#if WITH_EDITOR
void FVoxelNode_GetSurfaceAttributes::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FixupPins();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_GetSurfaceAttributes::FixupPins()
{
	VOXEL_FUNCTION_COUNTER();

	for (const auto& It : MakeCopy(GetNameToPin()))
	{
		if (It.Key == SurfacePin)
		{
			continue;
		}

		RemovePin(It.Key);
	}

	for (FVoxelSurfaceAttributeInfo& Attribute : Attributes)
	{
		while (FindPin(Attribute.Name))
		{
			Attribute.Name.SetNumber(Attribute.Name.GetNumber() + 1);
		}

		CreateOutputPin(Attribute.Type, Attribute.Name);
	}
}