// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFunctionLibrary.h"
#include "VoxelBuffer.h"
#include "Nodes/VoxelNode_UFunction.h"

void UVoxelFunctionLibrary::RaiseQueryErrorStatic(const FVoxelGraphNodeRef& Node, const UScriptStruct* QueryType)
{
	IVoxelNodeInterface::RaiseQueryErrorStatic(Node, QueryType);
}

void UVoxelFunctionLibrary::RaiseBufferErrorStatic(const FVoxelGraphNodeRef& Node)
{
	IVoxelNodeInterface::RaiseBufferErrorStatic(Node);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinType UVoxelFunctionLibrary::MakeType(const FProperty& Property)
{
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		if (StructProperty->Struct == StaticStructFast<FVoxelRuntimePinValue>())
		{
			return FVoxelPinType::MakeWildcard();
		}
		if (StructProperty->Struct->IsChildOf(StaticStructFast<FVoxelBuffer>()))
		{
			return FVoxelBuffer::FindInnerType_NotComplex(StructProperty->Struct).GetBufferType();
		}
	}

	return FVoxelPinType(Property);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelFunctionLibrary::FCachedFunction::FCachedFunction(const UFunction& Function)
	: Function(Function)
{
	NativeFunc = reinterpret_cast<FNativeFuncPtr>(Function.GetNativeFunc());

	ReturnProperty = Function.GetReturnProperty();

	// ReturnProperty should be the last property
	ensure(!ReturnProperty || !ReturnProperty->PropertyLinkNext);

	if (ReturnProperty)
	{
		ReturnPropertyType = MakeType(*ReturnProperty);

		if (ReturnPropertyType.IsWildcard())
		{
			Struct = StaticStructFast<FVoxelRuntimePinValue>();
		}
		else if (ReturnPropertyType.IsBuffer())
		{
			// We handle FVoxelComplexTerminalBuffer below
			Struct = FVoxelBuffer::Make(ReturnPropertyType.GetInnerType())->GetStruct();
		}
		else if (ReturnPropertyType.IsStruct())
		{
			Struct = ReturnPropertyType.GetStruct();
		}
	}

	if (Struct)
	{
		CppStructOps = Struct->GetCppStructOps();
		StructureSize = Struct->GetStructureSize();
		bStructHasZeroConstructor = Struct->GetCppStructOps()->HasZeroConstructor();
	}
}

void UVoxelFunctionLibrary::Call(
	const FVoxelNode_UFunction& Node,
	const FCachedFunction& CachedFunction,
	const FVoxelQuery& Query,
	const TConstVoxelArrayView<FVoxelRuntimePinValue*> Values)
{
	const FVoxelQueryScope Scope(Query);
	FVoxelNodeStatScope StatScope(Node, 0);

	void* ReturnMemory = nullptr;
	if (CachedFunction.ReturnProperty)
	{
		if (CachedFunction.Struct)
		{
			if (CachedFunction.Struct == StaticStructFast<FVoxelRuntimePinValue>())
			{
				checkVoxelSlow(Values.Last());
				ReturnMemory = Values.Last();
			}
			else
			{
				ReturnMemory = FVoxelMemory::Malloc(CachedFunction.StructureSize);

				if (CachedFunction.bStructHasZeroConstructor)
				{
					FMemory::Memzero(ReturnMemory, CachedFunction.StructureSize);
				}
				else
				{
					CachedFunction.CppStructOps->Construct(ReturnMemory);
				}

				if (CachedFunction.Struct == StaticStructFast<FVoxelComplexTerminalBuffer>())
				{
					static_cast<FVoxelComplexTerminalBuffer*>(ReturnMemory)->Initialize(CachedFunction.ReturnPropertyType.GetInnerType());
				}
			}
		}
		else
		{
			constexpr int32 Size = FMath::Max(sizeof(FName), sizeof(uint64));
			ReturnMemory = FMemory_Alloca(Size);
			FMemory::Memzero(ReturnMemory, Size);
		}
	}

	int32 Num = 1;
	if (StatScope.IsEnabled() ||
		AreVoxelStatsEnabled())
	{
		for (const FVoxelRuntimePinValue* Value : Values)
		{
			if (!Value ||
				!Value->IsValid())
			{
				continue;
			}

			if (Value->CanBeCastedTo<FVoxelBuffer>())
			{
				Num = FMath::Max(Num, Value->Get<FVoxelBuffer>().Num());
			}
		}

		StatScope.SetCount(Num);
	}

	{
		VOXEL_SCOPE_COUNTER_FORMAT("%s Num=%d", *CachedFunction.Function.GetName(), Num);
		VOXEL_SCOPE_COUNTER_FNAME(CachedFunction.Function.GetFName());

		FContext Context;
		Context.NodeRef = &Node.GetNodeRef();
		Context.Query = &Query;

#if VOXEL_DEBUG
		TVoxelArray<FName> AllOutputPins_Debug;
		for (const FProperty& Property : GetFunctionProperties(CachedFunction.Function))
		{
			if (!IsFunctionInput(Property))
			{
				AllOutputPins_Debug.Add(Property.GetFName());
			}
		}
		Context.AllOutputPins_Debug = AllOutputPins_Debug;
#endif

		FFrame Frame;
		Frame.Values = Values;
		CachedFunction.NativeFunc(reinterpret_cast<UObject*>(&Context), Frame, ReturnMemory);
	}

	if (!CachedFunction.ReturnProperty)
	{
		return;
	}

	if (CachedFunction.Struct == StaticStructFast<FVoxelRuntimePinValue>())
	{
		checkVoxelSlow(ReturnMemory == Values.Last());
		return;
	}

	checkVoxelSlow(Values.Last());

	FVoxelRuntimePinValue& ReturnValue = *Values.Last();
	ReturnValue.Type = CachedFunction.ReturnPropertyType;

	if (CachedFunction.Struct)
	{
		ReturnValue.SharedStructType = CachedFunction.Struct;
		ReturnValue.SharedStruct = MakeShareableStruct(CachedFunction.Struct, ReturnMemory);
	}
	else
	{
		switch (ReturnValue.Type.GetInternalType())
		{
		default: VOXEL_ASSUME(false);
		case EVoxelPinInternalType::Bool: ReturnValue.bBool = *static_cast<const bool*>(ReturnMemory); break;
		case EVoxelPinInternalType::Float: ReturnValue.Float = *static_cast<const float*>(ReturnMemory); break;
		case EVoxelPinInternalType::Double: ReturnValue.Double = *static_cast<const double*>(ReturnMemory); break;
		case EVoxelPinInternalType::Int32: ReturnValue.Int32 = *static_cast<const int32*>(ReturnMemory); break;
		case EVoxelPinInternalType::Int64: ReturnValue.Int64 = *static_cast<const int64*>(ReturnMemory); break;
		case EVoxelPinInternalType::Name: ReturnValue.Name = *static_cast<const FName*>(ReturnMemory); break;
		case EVoxelPinInternalType::Byte: ReturnValue.Byte = *static_cast<const uint8*>(ReturnMemory); break;
		case EVoxelPinInternalType::Class: ReturnValue.Class = *static_cast<UClass* const*>(ReturnMemory); break;
		}
	}

	if (StatScope.IsEnabled() &&
		ReturnValue.CanBeCastedTo<FVoxelBuffer>())
	{
		StatScope.SetCount(FMath::Max(Num, ReturnValue.Get<FVoxelBuffer>().Num()));
	}
}