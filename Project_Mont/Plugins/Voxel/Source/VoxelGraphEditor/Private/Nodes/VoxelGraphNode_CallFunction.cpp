// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_CallFunction.h"
#include "VoxelGraph.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelFunctionLibraryAsset.h"

const UVoxelTerminalGraph* UVoxelGraphNode_CallFunction::GetBaseTerminalGraph() const
{
	const UVoxelGraph* Graph = GetTypedOuter<UVoxelGraph>();
	if (!ensure(Graph))
	{
		return nullptr;
	}

	if (!bCallParent)
	{
		return Graph->FindTerminalGraph(Guid);
	}

	const UVoxelGraph* BaseGraph = Graph->GetBaseGraph_Unsafe();
	if (!BaseGraph)
	{
		return nullptr;
	}

	return BaseGraph->FindTerminalGraph(Guid);
}

void UVoxelGraphNode_CallFunction::AllocateDefaultPins()
{
	if (const UVoxelTerminalGraph* BaseTerminalGraph = GetBaseTerminalGraph())
	{
		CachedName = BaseTerminalGraph->GetDisplayName();

		OnFunctionMetaDataChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnTerminalGraphMetaDataChanged(*BaseTerminalGraph).Add(FOnVoxelGraphChanged::Make(OnFunctionMetaDataChangedPtr, this, [=]
		{
			RefreshNode();
		}));
	}

	Super::AllocateDefaultPins();
}

FText UVoxelGraphNode_CallFunction::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	if (!GetBaseTerminalGraph())
	{
		return FText::FromString(CachedName);
	}

	if (!bCallParent)
	{
		return Super::GetNodeTitle(TitleType);
	}

	return FText::FromString("Call Parent: " + Super::GetNodeTitle(TitleType).ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_CallFunction::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (GetBaseTerminalGraph())
	{
		CachedName = GetNodeTitle(ENodeTitleType::FullTitle).ToString();
	}
}

void UVoxelGraphNode_CallFunction::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelGraph* Graph = GetTypedOuter<UVoxelGraph>();
	if (!ensure(Graph))
	{
		return;
	}

	if (Graph->FindTerminalGraph(Guid))
	{
		return;
	}

	// Try to find a matching name
	for (const FGuid& TerminalGraphGuid : Graph->GetTerminalGraphs())
	{
		const UVoxelTerminalGraph& TerminalGraph = Graph->FindTerminalGraphChecked(TerminalGraphGuid);
		if (TerminalGraph.GetDisplayName() != CachedName)
		{
			continue;
		}

		Guid = TerminalGraphGuid;
		return;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const UVoxelTerminalGraph* UVoxelGraphNode_CallExternalFunction::GetBaseTerminalGraph() const
{
	if (!FunctionLibrary)
	{
		return nullptr;
	}

	return FunctionLibrary->GetGraph().FindTerminalGraph(Guid);
}

void UVoxelGraphNode_CallExternalFunction::AllocateDefaultPins()
{
	if (const UVoxelTerminalGraph* BaseTerminalGraph = GetBaseTerminalGraph())
	{
		CachedName = BaseTerminalGraph->GetDisplayName();

		OnFunctionMetaDataChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnTerminalGraphMetaDataChanged(*BaseTerminalGraph).Add(FOnVoxelGraphChanged::Make(OnFunctionMetaDataChangedPtr, this, [=]
		{
			RefreshNode();
		}));
	}

	Super::AllocateDefaultPins();
}

FText UVoxelGraphNode_CallExternalFunction::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	if (GetBaseTerminalGraph())
	{
		return Super::GetNodeTitle(TitleType);
	}

	return FText::FromString(CachedName);
}