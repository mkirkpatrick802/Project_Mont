// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Nodes/VoxelArrayNodes.h"
#include "VoxelBuffer.h"
#include "VoxelBufferBuilder.h"
#include "Utilities/VoxelBufferGatherer.h"

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_ArrayLength, Result)
{
	const TValue<FVoxelBuffer> Values = GetNodeRuntime().Get<FVoxelBuffer>(ValuesPin, Query);

	return VOXEL_ON_COMPLETE(Values)
	{
		return Values->Num();
	};
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_ArrayLength::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::AllBufferArrays();
}

void FVoxelNode_ArrayLength::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	Pin.SetType(NewType);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_ArrayGetItem, Result)
{
	const TValue<FVoxelBuffer> Values = GetNodeRuntime().Get<FVoxelBuffer>(ValuesPin, Query);

	if (AreTemplatePinsBuffers())
	{
		const TValue<FVoxelInt32Buffer> Indices = GetNodeRuntime().Get<FVoxelInt32Buffer>(IndexPin, Query);
		return VOXEL_ON_COMPLETE(Values, Indices)
		{
			for (int32 Index = 0; Index < Indices.Num(); Index++)
			{
				if (!Values->IsValidIndex(Indices[Index]))
				{
					VOXEL_MESSAGE(Error, "{0}: Invalid Index {1}. Values.Num={2}", this, Indices[Index], Values->Num());
					return {};
				}
			}

			return FVoxelRuntimePinValue::Make(FVoxelBufferGatherer::Gather(*Values, Indices), ReturnPinType);
		};
	}
	else
	{
		const TValue<int32> Index = GetNodeRuntime().Get<int32>(IndexPin, Query);
		return VOXEL_ON_COMPLETE(Values, Index)
		{
			const int32 NumValues = Values->Num();
			if (!Values->IsValidIndex(Index))
			{
				VOXEL_MESSAGE(Error, "{0}: Invalid Index {1}. Values.Num={2}", this, Index, NumValues);
				return {};
			}

			return Values->GetGeneric(Index);
		};
	}
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_ArrayGetItem::GetPromotionTypes(const FVoxelPin& Pin) const
{
	if (Pin.Name == IndexPin)
	{
		FVoxelPinTypeSet OutTypes;
		OutTypes.Add(FVoxelPinType::Make<int32>());
		OutTypes.Add(FVoxelPinType::Make<FVoxelInt32Buffer>());
		return OutTypes;
	}
	else if (Pin.Name == ValuesPin)
	{
		return FVoxelPinTypeSet::AllBufferArrays();
	}
	else
	{
		ensure(Pin.Name == ResultPin);
		return FVoxelPinTypeSet::All();
	}
}

void FVoxelNode_ArrayGetItem::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	if (Pin.Name == IndexPin)
	{
		GetPin(IndexPin).SetType(NewType);

		if (NewType.IsBuffer())
		{
			GetPin(ResultPin).SetType(GetPin(ResultPin).GetType().GetBufferType());
		}
		else
		{
			GetPin(ResultPin).SetType(GetPin(ResultPin).GetType().GetInnerType());
		}
	}
	else if (Pin.Name == ValuesPin)
	{
		GetPin(ValuesPin).SetType(NewType.GetBufferType().WithBufferArray(true));

		if (GetPin(IndexPin).GetType().IsBuffer())
		{
			GetPin(ResultPin).SetType(NewType.GetBufferType().WithBufferArray(false));
		}
		else
		{
			GetPin(ResultPin).SetType(NewType.GetInnerType());
		}
	}
	else
	{
		ensure(Pin.Name == ResultPin);

		GetPin(ValuesPin).SetType(NewType.GetBufferType().WithBufferArray(true));
		Pin.SetType(NewType);

		if (NewType.IsBuffer())
		{
			GetPin(IndexPin).SetType(FVoxelPinType::Make<FVoxelInt32Buffer>());
		}
		else
		{
			GetPin(IndexPin).SetType(FVoxelPinType::Make<int32>());
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_MakeArray, Result)
{
	const TVoxelArray<FVoxelFutureValue> Items = GetNodeRuntime().Get(FVoxelVariadicPinRef(ItemPins), Query);

	return VOXEL_ON_COMPLETE(Items)
	{
		const int32 NumValues = Items.Num();
		if (NumValues == 0)
		{
			return {};
		}

		const FVoxelPinType Type = GetNodeRuntime().GetPinData(ResultPin).Type;

		FVoxelBufferBuilder BufferBuilder(Type.GetInnerType());
		for (const FVoxelFutureValue& Item : Items)
		{
			BufferBuilder.Add(Item.GetValue_CheckCompleted());
		}

		return FVoxelRuntimePinValue::Make(BufferBuilder.MakeBuffer(), Type.GetBufferType().WithBufferArray(false));
	};
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_MakeArray::GetPromotionTypes(const FVoxelPin& Pin) const
{
	if (Pin.Name == ResultPin)
	{
		return FVoxelPinTypeSet::AllBufferArrays();
	}
	else
	{
		return FVoxelPinTypeSet::AllUniforms();
	}
}

void FVoxelNode_MakeArray::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(ResultPin).SetType(NewType.GetBufferType().WithBufferArray(true));

	const TVoxelArray<FVoxelPinRef>& Pins = GetVariadicPinPinNames(FVoxelVariadicPinRef(ItemPins));
	for (const FVoxelPinRef& ItemPin : Pins)
	{
		GetPin(ItemPin).SetType(NewType.GetInnerType());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FName FVoxelNode_MakeArray::FDefinition::Variadic_AddPinTo(const FName VariadicPinName)
{
	ensure(!VariadicPinName.IsNone());
	const FName NewPin = Node.Variadic_AddPin(VariadicPinName);

	const FVoxelPinType ResultPinType = Node.GetPin(Node.ResultPin).GetType();
	if (!ResultPinType.IsWildcard())
	{
		Node.GetPin(FVoxelPinRef(NewPin)).SetType(ResultPinType.GetInnerType());
	}

	return NewPin;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_AddToArray, Result)
{
	const TValue<FVoxelBuffer> Array = GetNodeRuntime().Get<FVoxelBuffer>(ArrayPin, Query);
	const TVoxelArray<FVoxelFutureValue> Items = GetNodeRuntime().Get(FVoxelVariadicPinRef(ItemPins), Query);

	return VOXEL_ON_COMPLETE(Array, Items)
	{
		const FVoxelPinType Type = GetNodeRuntime().GetPinData(ResultPin).Type;

		const int32 NumValues = Items.Num();
		if (NumValues == 0)
		{
			return FVoxelRuntimePinValue::Make(Array, Type);
		}

		FVoxelBufferBuilder BufferBuilder(Type.GetInnerType());
		BufferBuilder.Append(*Array, Array->Num());
		for (const FVoxelFutureValue& Item : Items)
		{
			BufferBuilder.Add(Item.GetValue_CheckCompleted());
		}

		return FVoxelRuntimePinValue::Make(BufferBuilder.MakeBuffer(), Type.GetBufferType().WithBufferArray(false));
	};
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_AddToArray::GetPromotionTypes(const FVoxelPin& Pin) const
{
	if (Pin.Name == ArrayPin ||
		Pin.Name == ResultPin)
	{
		return FVoxelPinTypeSet::AllBufferArrays();
	}
	else
	{
		return FVoxelPinTypeSet::AllUniforms();
	}
}

void FVoxelNode_AddToArray::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(ArrayPin).SetType(NewType.GetBufferType().WithBufferArray(true));
	GetPin(ResultPin).SetType(NewType.GetBufferType().WithBufferArray(true));

	const TVoxelArray<FVoxelPinRef>& Pins = GetVariadicPinPinNames(FVoxelVariadicPinRef(ItemPins));
	for (const FVoxelPinRef& ItemPin : Pins)
	{
		GetPin(ItemPin).SetType(NewType.GetInnerType());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FName FVoxelNode_AddToArray::FDefinition::Variadic_AddPinTo(const FName VariadicPinName)
{
	ensure(!VariadicPinName.IsNone());
	const FName NewPin = Node.Variadic_AddPin(VariadicPinName);

	const FVoxelPinType ResultPinType = Node.GetPin(Node.ResultPin).GetType();
	if (!ResultPinType.IsWildcard())
	{
		Node.GetPin(FVoxelPinRef(NewPin)).SetType(ResultPinType.GetInnerType());
	}

	return NewPin;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_NODE_COMPUTE(FVoxelNode_AppendArray, Result)
{
	const TValue<FVoxelBuffer> A = GetNodeRuntime().Get<FVoxelBuffer>(APin, Query);
	const TValue<FVoxelBuffer> B = GetNodeRuntime().Get<FVoxelBuffer>(BPin, Query);

	return VOXEL_ON_COMPLETE(A, B)
	{
		const FVoxelPinType Type = GetNodeRuntime().GetPinData(ResultPin).Type;

		FVoxelBufferBuilder BufferBuilder(Type.GetInnerType());
		BufferBuilder.Append(*A, A->Num());
		BufferBuilder.Append(*B, B->Num());

		return FVoxelRuntimePinValue::Make(BufferBuilder.MakeBuffer(), Type.GetBufferType().WithBufferArray(false));
	};
}

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode_AppendArray::GetPromotionTypes(const FVoxelPin& Pin) const
{
	return FVoxelPinTypeSet::AllBufferArrays();
}

void FVoxelNode_AppendArray::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	GetPin(APin).SetType(NewType.GetBufferType().WithBufferArray(true));
	GetPin(BPin).SetType(NewType.GetBufferType().WithBufferArray(true));
	GetPin(ResultPin).SetType(NewType.GetBufferType().WithBufferArray(true));
}
#endif