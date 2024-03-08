// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphMessageTokens.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Logging/TokenizedMessage.h"

uint32 FVoxelMessageToken_NodeRef::GetHash() const
{
	return GetTypeHash(NodeRef);
}

FString FVoxelMessageToken_NodeRef::ToString() const
{
	return NodeRef.EdGraphNodeTitle.ToString();
}

TSharedRef<IMessageToken> FVoxelMessageToken_NodeRef::GetMessageToken() const
{
	ensure(IsInGameThread());

#if WITH_EDITOR
	return FActionToken::Create(
		FText::FromString(ToString()),
		FText::FromString("Go to " + ToString()),
		MakeLambdaDelegate([NodeRef = NodeRef]
		{
			const UEdGraphNode* GraphNode = NodeRef.GetGraphNode_EditorOnly();
			if (!GraphNode)
			{
				return;
			}

			FVoxelUtilities::FocusObject(GraphNode);
		}));
#else
	return Super::GetMessageToken();
#endif
}

void FVoxelMessageToken_NodeRef::GetObjects(TSet<const UObject*>& OutObjects) const
{
	OutObjects.Add(NodeRef.GetNodeTerminalGraph(FOnVoxelGraphChanged::Null()));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint32 FVoxelMessageToken_PinRef::GetHash() const
{
	return GetTypeHash(PinRef);
}

FString FVoxelMessageToken_PinRef::ToString() const
{
	return PinRef.NodeRef.EdGraphNodeTitle.ToString() + "."+ PinRef.PinName.ToString();
}

TSharedRef<IMessageToken> FVoxelMessageToken_PinRef::GetMessageToken() const
{
	ensure(IsInGameThread());

#if WITH_EDITOR
	return FActionToken::Create(
		FText::FromString(ToString()),
		FText::FromString("Go to " + ToString()),
		MakeLambdaDelegate([PinRef = PinRef]
		{
			const UEdGraphNode* GraphNode = PinRef.NodeRef.GetGraphNode_EditorOnly();
			if (!GraphNode)
			{
				return;
			}

			FVoxelUtilities::FocusObject(GraphNode);
		}));
#else
	return Super::GetMessageToken();
#endif
}

void FVoxelMessageToken_PinRef::GetObjects(TSet<const UObject*>& OutObjects) const
{
	OutObjects.Add(PinRef.NodeRef.GetNodeTerminalGraph(FOnVoxelGraphChanged::Null()));
}