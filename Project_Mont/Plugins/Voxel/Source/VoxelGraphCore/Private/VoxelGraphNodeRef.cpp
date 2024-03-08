// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNodeRef.h"
#include "VoxelGraph.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphMessageTokens.h"
#include "VoxelTerminalGraphRuntime.h"
#if WITH_EDITOR
#include "EdGraph/EdGraph.h"
#endif

const FName FVoxelGraphConstants::NodeId_Preview = "PreviewNode";
const FName FVoxelGraphConstants::PinName_EnableNode = "EnableNode";
const FName FVoxelGraphConstants::PinName_GraphParameter = "GraphParameter";

FName FVoxelGraphConstants::GetOutputNodeId(const FGuid& Guid)
{
	return FName("Output." + Guid.ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelTerminalGraphRef::FVoxelTerminalGraphRef(const UVoxelTerminalGraph* TerminalGraph)
{
	if (!TerminalGraph)
	{
		return;
	}

	Graph = &TerminalGraph->GetGraph();
	TerminalGraphGuid = TerminalGraph->GetGuid();
}

FVoxelTerminalGraphRef::FVoxelTerminalGraphRef(const UVoxelTerminalGraph& TerminalGraph)
	: FVoxelTerminalGraphRef(&TerminalGraph)
{
}

const UVoxelTerminalGraph* FVoxelTerminalGraphRef::GetTerminalGraph(const FOnVoxelGraphChanged& OnChanged) const
{
	ensure(IsInGameThread());

	if (!Graph.IsValid())
	{
		ensureVoxelSlow(IsExplicitlyNull());
		return nullptr;
	}

#if WITH_EDITOR
	GVoxelGraphTracker->OnTerminalGraphChanged(*Graph).Add(OnChanged);
#endif

	const UVoxelTerminalGraph* TerminalGraph = Graph->FindTerminalGraph(TerminalGraphGuid);
	ensure(TerminalGraph);
	return TerminalGraph;
}

#if WITH_EDITOR
UEdGraphNode* FVoxelGraphNodeRef::GetGraphNode_EditorOnly() const
{
	VOXEL_FUNCTION_COUNTER();
	ensure(IsInGameThread());

	if (EdGraphNodeName.IsNone())
	{
		return nullptr;
	}

	const UVoxelTerminalGraph* TerminalGraph = GetNodeTerminalGraph(FOnVoxelGraphChanged::Null());
	if (!TerminalGraph)
	{
		return nullptr;
	}

	for (UEdGraphNode* Node : TerminalGraph->GetEdGraph().Nodes)
	{
		if (ensure(Node) &&
			Node->GetFName() == EdGraphNodeName)
		{
			return Node;
		}
	}

	ensureVoxelSlow(false);
	return nullptr;
}
#endif

bool FVoxelGraphNodeRef::IsDeleted() const
{
	if (EdGraphNodeName.IsNone())
	{
		return false;
	}

	const UVoxelTerminalGraph* TerminalGraph = GetNodeTerminalGraph(FOnVoxelGraphChanged::Null());
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	return !TerminalGraph->GetRuntime().GetSerializedGraph().NodeNameToNode.Contains(EdGraphNodeName);
}

void FVoxelGraphNodeRef::AppendString(FWideStringBuilderBase& Out) const
{
	if (!EdGraphNodeTitle.IsNone())
	{
		EdGraphNodeTitle.AppendString(Out);
	}
	else
	{
		NodeId.AppendString(Out);
	}
}

bool FVoxelGraphNodeRef::NetSerialize(FArchive& Ar, UPackageMap& Map)
{
	if (Ar.IsSaving())
	{
		UObject* Object = ConstCast(TerminalGraphRef.Graph.Get());
		ensure(Object);

		if (!ensure(Map.SerializeObject(Ar, UVoxelGraph::StaticClass(), Object)))
		{
			return false;
		}

		Ar << TerminalGraphRef.TerminalGraphGuid;
		Ar << NodeId;
		ensure(TemplateInstance == 0);

		return true;
	}
	else if (Ar.IsLoading())
	{
		UObject* Object = nullptr;
		if (!ensure(Map.SerializeObject(Ar, UVoxelGraph::StaticClass(), Object)) ||
			!ensure(Object) ||
			!ensure(Object->IsA<UVoxelGraph>()))
		{
			return false;
		}

		TerminalGraphRef.Graph = CastChecked<UVoxelGraph>(Object);
		Ar << TerminalGraphRef.TerminalGraphGuid;
		Ar << NodeId;
		ensure(TemplateInstance == 0);

		return true;
	}
	else
	{
		ensure(false);
		return false;
	}
}

FVoxelGraphNodeRef FVoxelGraphNodeRef::WithSuffix(const FString& Suffix) const
{
	FVoxelGraphNodeRef Result = *this;
	Result.NodeId += "_" + Suffix;
	Result.EdGraphNodeTitle += " (" + Suffix + ")";
	return Result;
}

TSharedRef<FVoxelMessageToken> FVoxelGraphNodeRef::CreateMessageToken() const
{
	const TSharedRef<FVoxelMessageToken_NodeRef> Result = MakeVoxelShared<FVoxelMessageToken_NodeRef>();
	Result->NodeRef = *this;
	return Result;
}

const UVoxelTerminalGraph* FVoxelGraphNodeRef::GetNodeTerminalGraph(const FOnVoxelGraphChanged& OnChanged) const
{
	ensure(IsInGameThread());

	const UVoxelTerminalGraph* TerminalGraph = TerminalGraphRef.GetTerminalGraph(OnChanged);
	if (!TerminalGraph)
	{
		return nullptr;
	}

	if (!TerminalGraph->IsMainTerminalGraph())
	{
		// Function overrides never implicitly use their parent outputs
		return TerminalGraph;
	}

	FString OutputGuidString = NodeId.ToString();
	if (!OutputGuidString.RemoveFromStart("Output."))
	{
		return TerminalGraph;
	}

	FGuid OutputGuid;
	if (!ensure(FGuid::ParseExact(OutputGuidString, EGuidFormats::Digits, OutputGuid)))
	{
		return nullptr;
	}

#if WITH_EDITOR
	GVoxelGraphTracker->OnBaseTerminalGraphsChanged(*TerminalGraph).Add(OnChanged);
#endif

	for (const UVoxelTerminalGraph* BaseTerminalGraph : TerminalGraph->GetBaseTerminalGraphs())
	{
#if WITH_EDITOR
		GVoxelGraphTracker->OnTerminalGraphTranslated(*TerminalGraph).Add(OnChanged);
#endif

		if (BaseTerminalGraph->GetRuntime().GetSerializedGraph().OutputGuids.Contains(OutputGuid))
		{
			return BaseTerminalGraph;
		}
	}

	// Returning default value is fine VOXEL_MESSAGE(Error, "Missing output {0} on {1}", OutputName, TerminalGraph);
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphPinRef::ToString() const
{
	ensure(IsInGameThread());

	const UVoxelTerminalGraph* TerminalGraph = NodeRef.GetNodeTerminalGraph(FOnVoxelGraphChanged::Null());
	return FString::Printf(TEXT("%s.%s.%s"),
		TerminalGraph ? *TerminalGraph->GetPathName() : TEXT("<null>"),
		*NodeRef.EdGraphNodeTitle.ToString(),
		*PinName.ToString());
}

const UVoxelTerminalGraph* FVoxelGraphPinRef::GetNodeTerminalGraph(const FOnVoxelGraphChanged& OnChanged) const
{
	return NodeRef.GetNodeTerminalGraph(OnChanged);
}

TSharedRef<FVoxelMessageToken> FVoxelGraphPinRef::CreateMessageToken() const
{
	const TSharedRef<FVoxelMessageToken_PinRef> Result = MakeVoxelShared<FVoxelMessageToken_PinRef>();
	Result->PinRef = *this;
	return Result;
}