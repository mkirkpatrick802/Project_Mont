// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphCompileScope.h"
#include "VoxelMessage.h"
#include "VoxelTerminalGraph.h"
#include "EdGraph/EdGraph.h"

FVoxelGraphCompileScope* GVoxelGraphCompileScope = nullptr;

FVoxelGraphCompileScope::FVoxelGraphCompileScope(
	const UVoxelTerminalGraph& TerminalGraph,
	const bool bEnableLogging)
	: TerminalGraph(TerminalGraph)
{
	PreviousScope = GVoxelGraphCompileScope;
	GVoxelGraphCompileScope = this;

	ScopedMessageConsumer = MakeVoxelUnique<FVoxelScopedMessageConsumer>([=](const TSharedRef<FVoxelMessage>& Message)
	{
		if (Message->GetSeverity() == EVoxelMessageSeverity::Error)
		{
			bHasError = true;
		}

		Messages.Add(Message);

		if (!bEnableLogging)
		{
			return;
		}

		const FString GraphPath = this->TerminalGraph.GetPathName();

		switch (Message->GetSeverity())
		{
		default: ensure(false);
		case EVoxelMessageSeverity::Info:
		{
			LOG_VOXEL(Log, "%s: %s", *GraphPath, *Message->ToString());
		}
		break;
		case EVoxelMessageSeverity::Warning:
		{
			LOG_VOXEL(Warning, "%s: %s", *GraphPath, *Message->ToString());
		}
		break;
		case EVoxelMessageSeverity::Error:
		{
			if (IsRunningCookCommandlet() ||
				IsRunningCookOnTheFly())
			{
				// Don't fail cooking
				LOG_VOXEL(Warning, "%s: %s", *GraphPath, *Message->ToString());
			}
			else
			{
				LOG_VOXEL(Error, "%s: %s", *GraphPath, *Message->ToString());
			}
		}
		break;
		}
	});
}

FVoxelGraphCompileScope::~FVoxelGraphCompileScope()
{
	ensure(GVoxelGraphCompileScope == this);
	GVoxelGraphCompileScope = PreviousScope;
}