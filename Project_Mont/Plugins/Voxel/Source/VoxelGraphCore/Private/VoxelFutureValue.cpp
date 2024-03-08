// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelFutureValue.h"
#include "VoxelFutureValueState.h"

FVoxelDummyFutureValue FVoxelFutureValue::MakeDummy(const FName DebugName)
{
	const TSharedRef<FVoxelFutureValueStateImpl> State = MakeVoxelShared<FVoxelFutureValueStateImpl>(FVoxelPinType::Make<FVoxelFutureValueDummyStruct>(), DebugName);
	return TVoxelFutureValue<FVoxelFutureValueDummyStruct>(FVoxelFutureValue(State));
}

void FVoxelFutureValue::MarkDummyAsCompleted() const
{
	check(IsValid());
	check(GetParentType().Is<FVoxelFutureValueDummyStruct>());

	State->GetStateImpl().SetValue(
		FVoxelRuntimePinValue::Make<FVoxelFutureValueDummyStruct>(
			MakeVoxelShared<FVoxelFutureValueDummyStruct>()));
}