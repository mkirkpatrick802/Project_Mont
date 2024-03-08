// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTaskHelpers.h"
#include "VoxelTaskGroupScope.h"
#include "VoxelFutureValueState.h"

void FVoxelTaskHelpers::StartAsyncTaskGeneric(
	const FName Name,
	const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
	const FVoxelPinType& Type,
	TVoxelUniqueFunction<FVoxelFutureValue()> MakeFuture,
	TFunction<void(const FVoxelRuntimePinValue&)> Callback)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FVoxelTaskGroup> Group = FVoxelTaskGroup::Create(
		Name,
		FVoxelTaskPriority::MakeTop(),
		MakeVoxelShared<FVoxelTaskReferencer>(Name),
		TerminalGraphInstance);

	FVoxelTaskGroupScope Scope;
	if (!ensure(Scope.Initialize(*Group)))
	{
		return;
	}

	const FVoxelFutureValue Future =
		MakeVoxelTask()
		.Execute(Type, MoveTemp(MakeFuture));

	MakeVoxelTask()
	.Dependency(Future)
	.Execute([=]
	{
		// Keep group alive
		(void)Group;

		Callback(Future.GetValue_CheckCompleted());
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelTaskHelpers::TryRunSynchronouslyGeneric(
	const TSharedRef<const FVoxelTerminalGraphInstance>& TerminalGraphInstance,
	const TFunctionRef<void()> Lambda,
	FString* OutError)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedRef<FVoxelTaskGroup> Group = FVoxelTaskGroup::CreateSynchronous(
		STATIC_FNAME("Synchronous"),
		MakeVoxelShared<FVoxelTaskReferencer>(STATIC_FNAME("Synchronous")),
		TerminalGraphInstance);

	FVoxelTaskGroupScope Scope;
	if (!ensure(Scope.Initialize(*Group)))
	{
		// Exiting
		if (OutError)
		{
			*OutError = "Runtime is being destroyed";
		}
		return false;
	}

	Lambda();

	return Group->TryRunSynchronously(OutError);
}