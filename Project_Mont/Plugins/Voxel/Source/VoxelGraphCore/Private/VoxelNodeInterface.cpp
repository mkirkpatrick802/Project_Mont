// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelNodeInterface.h"
#include "VoxelQuery.h"
#include "VoxelMessage.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelTerminalGraphInstance.h"

VOXEL_RUN_ON_STARTUP_GAME(RegisterGatherCallstack)
{
	GVoxelMessageManager->GatherCallstacks.Add([](const TSharedRef<FVoxelMessage>& Message)
	{
		const FVoxelQueryScope* QueryScope = FVoxelQueryScope::TryGet();
		if (!QueryScope)
		{
			return;
		}

		if (!QueryScope->Query)
		{
			if (!QueryScope->TerminalGraphInstance)
			{
				return;
			}

			if (QueryScope->TerminalGraphInstance->Callstack->IsValid())
			{
				Message->AddText(" ");
				Message->AddToken(QueryScope->TerminalGraphInstance->Callstack->CreateMessageToken());
			}

			return;
		}

		if (QueryScope->Query->GetCallstack().IsValid())
		{
			Message->AddText(" ");
			Message->AddToken(QueryScope->Query->GetCallstack().CreateMessageToken());
		}
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void IVoxelNodeInterface::RaiseQueryErrorStatic(const FVoxelGraphNodeRef& Node, const UScriptStruct* QueryType)
{
	VOXEL_MESSAGE(Error, "{0}: {1} is required but not provided by callee", Node, QueryType->GetName());
}

void IVoxelNodeInterface::RaiseBufferErrorStatic(const FVoxelGraphNodeRef& Node)
{
	VOXEL_MESSAGE(Error, "{0}: Inputs have different buffer sizes", Node);
}