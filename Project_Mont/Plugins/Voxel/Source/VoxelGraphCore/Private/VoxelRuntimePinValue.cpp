// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelRuntimePinValue.h"
#include "VoxelBuffer.h"

FVoxelRuntimePinValue::FVoxelRuntimePinValue(const FVoxelPinType& Type)
	: FVoxelRuntimePinValue()
{
	this->Type = Type;

	check(!Type.IsWildcard());
	check(!Type.GetInnerType().IsObject());

	if (Type.IsBuffer())
	{
		TSharedRef<FVoxelBuffer> Buffer = FVoxelBuffer::Make(Type.GetInnerType());
		SharedStructType = Buffer->GetStruct();
		SharedStruct = MakeSharedVoidRef(MoveTemp(Buffer));
	}
	else if (Type.IsStruct())
	{
		SharedStructType = Type.GetStruct();
		SharedStruct = MakeSharedStruct(SharedStructType);
	}
}

FVoxelRuntimePinValue FVoxelRuntimePinValue::MakeStruct(const FConstVoxelStructView Struct)
{
	FVoxelRuntimePinValue Result;
	Result.Type = FVoxelPinType::MakeStruct(Struct.GetStruct());
	Result.SharedStructType = Struct.GetStruct();
	Result.SharedStruct = Struct.MakeSharedCopy();
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelRuntimePinValue FVoxelRuntimePinValue::Make(
	const TSharedRef<const FVoxelBuffer>& Value,
	const FVoxelPinType& BufferType)
{
	checkVoxelSlow(BufferType.IsBuffer());

	FVoxelRuntimePinValue Result;
	Result.Type = BufferType;
	Result.SharedStructType = Value->GetStruct();
	Result.SharedStruct = MakeSharedVoidRef(Value);
	return Result;
}

FVoxelRuntimePinValue FVoxelRuntimePinValue::Make(
	const TSharedRef<FVoxelBuffer>& Value,
	const FVoxelPinType& BufferType)
{
	return Make(StaticCastSharedRef<const FVoxelBuffer>(Value), BufferType);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelRuntimePinValue::IsValidValue_Slow() const
{
	if (!Type.IsValid())
	{
		return false;
	}

	if (!Type.CanBeCastedTo<FVoxelBuffer>())
	{
		return true;
	}

	return Get<FVoxelBuffer>().IsValid_Slow();
}