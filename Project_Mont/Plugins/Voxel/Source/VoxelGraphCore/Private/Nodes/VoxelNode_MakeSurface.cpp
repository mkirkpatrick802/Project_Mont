// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelNode_MakeSurface.h"
#include "Material/VoxelMaterialBlending.h"

void FVoxelNode_MakeSurface::PostSerialize()
{
	FixupPins();

	Super::PostSerialize();
}

FVoxelComputeValue FVoxelNode_MakeSurface::CompileCompute(const FName PinName) const
{
	return [this](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));
		FVoxelQueryScope Scope(Query);

		// Might be null if we're just querying the bounds
		const TSharedPtr<const FVoxelSurfaceQueryParameter> SurfaceQueryParameter = Query.GetParameters().FindShared<FVoxelSurfaceQueryParameter>();

		TVoxelInlineArray<FVoxelFutureValue, 16> Futures;

		const TValue<FVoxelBox> FutureBounds = GetNodeRuntime().Get(BoundsPin, Query);
		Futures.Add(FutureBounds);

		TVoxelMap<FName, TValue<FVoxelBuffer>> NameToFutureAttribute;
		if (SurfaceQueryParameter)
		{
			NameToFutureAttribute.Reserve(SurfaceQueryParameter->AttributesToCompute.Num());

			for (const FName Name : SurfaceQueryParameter->AttributesToCompute)
			{
				if (!GetNodeRuntime().GetNameToPinData().Contains(Name))
				{
					continue;
				}

				const TValue<FVoxelBuffer> FutureAttribute = GetNodeRuntime().Get<FVoxelBuffer>(FVoxelPinRef(Name), Query);
				Futures.Add(FutureAttribute);
				NameToFutureAttribute.Add_CheckNew(Name, FutureAttribute);
			}
		}

		return
			MakeVoxelTask()
			.Dependencies(Futures)
			.Execute<FVoxelSurface>([=]
			{
				const TSharedRef<FVoxelSurface> Surface = MakeVoxelShared<FVoxelSurface>();
				Surface->Bounds = FutureBounds.Get_CheckCompleted();
				if (SurfaceQueryParameter)
				{
					Surface->AttributesToCompute = SurfaceQueryParameter->AttributesToCompute;
				}

				int32 Num = 1;

				Surface->NameToAttribute.Reserve(NameToFutureAttribute.Num());
				for (const auto& It : NameToFutureAttribute)
				{
					const TSharedRef<const FVoxelBuffer> Attribute = It.Value.GetShared_CheckCompleted();
					if (!FVoxelBufferAccessor::MergeNum(Num, *Attribute))
					{
						RaiseBufferError();
						continue;
					}

					Surface->NameToAttribute.Add_CheckNew(It.Key, Attribute);
				}

				return Surface;
			});
	};
}

#if WITH_EDITOR
void FVoxelNode_MakeSurface::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	FixupPins();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode_MakeSurface::FixupPins()
{
	VOXEL_FUNCTION_COUNTER();

	for (const auto& It : MakeCopy(GetNameToPin()))
	{
		if (It.Key == SurfacePin ||
			It.Key == BoundsPin)
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

		CreateInputPin(Attribute.Type, Attribute.Name);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelNode_Make2DSurface::FVoxelNode_Make2DSurface()
{
	Attributes.Add(FVoxelSurfaceAttributeInfo
	{
		FVoxelSurfaceAttributes::Height,
		FVoxelPinType::Make<FVoxelFloatBuffer>()
	});

	Attributes.Add(FVoxelSurfaceAttributeInfo
	{
		FVoxelSurfaceAttributes::Material,
		FVoxelPinType::Make<FVoxelMaterialBlendingBuffer>()
	});

	FixupPins();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelNode_Make3DSurface::FVoxelNode_Make3DSurface()
{
	Attributes.Add(FVoxelSurfaceAttributeInfo
	{
		FVoxelSurfaceAttributes::Distance,
		FVoxelPinType::Make<FVoxelFloatBuffer>()
	});

	Attributes.Add(FVoxelSurfaceAttributeInfo
	{
		FVoxelSurfaceAttributes::Material,
		FVoxelPinType::Make<FVoxelMaterialBlendingBuffer>()
	});

	FixupPins();
}