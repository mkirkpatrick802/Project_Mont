// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphNodeRef.generated.h"

class UVoxelGraph;
class UVoxelTerminalGraph;
struct FVoxelGraphNodeRef;
struct FOnVoxelGraphChanged;

struct VOXELGRAPHCORE_API FVoxelGraphConstants
{
	static const FName NodeId_Preview;
	static const FName PinName_EnableNode;
	static const FName PinName_GraphParameter;

	static FName GetOutputNodeId(const FGuid& Guid);
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelTerminalGraphRef
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<const UVoxelGraph> Graph;

	UPROPERTY()
	FGuid TerminalGraphGuid;

	FVoxelTerminalGraphRef() = default;
	FVoxelTerminalGraphRef(
		const TWeakObjectPtr<const UVoxelGraph>& Graph,
		const FGuid& TerminalGraphGuid)
		: Graph(Graph)
		, TerminalGraphGuid(TerminalGraphGuid)
	{
	}
	explicit FVoxelTerminalGraphRef(const UVoxelTerminalGraph* TerminalGraph);
	explicit FVoxelTerminalGraphRef(const UVoxelTerminalGraph& TerminalGraph);

	const UVoxelTerminalGraph* GetTerminalGraph(const FOnVoxelGraphChanged& OnChanged) const;

	FORCEINLINE bool IsExplicitlyNull() const
	{
		return
			Graph.IsExplicitlyNull() &&
			!TerminalGraphGuid.IsValid();
	}

	FORCEINLINE bool operator==(const FVoxelTerminalGraphRef& Other) const
	{
		return
			Graph == Other.Graph &&
			TerminalGraphGuid == Other.TerminalGraphGuid;
	}
	FORCEINLINE bool operator!=(const FVoxelTerminalGraphRef& Other) const
	{
		return
			Graph != Other.Graph ||
			TerminalGraphGuid != Other.TerminalGraphGuid;
	}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FVoxelTerminalGraphRef& Key)
	{
		Ar << Key.Graph;
		Ar << Key.TerminalGraphGuid;
		return Ar;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelTerminalGraphRef& Key)
	{
		return FVoxelUtilities::MurmurHashMulti(
			GetTypeHash(Key.Graph),
			GetTypeHash(Key.TerminalGraphGuid));
	}
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphNodeRef
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FVoxelTerminalGraphRef TerminalGraphRef;

	UPROPERTY()
	FName NodeId;

	UPROPERTY()
	int32 TemplateInstance = 0;

public:
	// Debug only
	UPROPERTY()
	FName EdGraphNodeTitle;

	// Debug only
	UPROPERTY()
	FName EdGraphNodeName;

public:
	FVoxelGraphNodeRef() = default;

	FVoxelGraphNodeRef(
		const FVoxelTerminalGraphRef& TerminalGraphRef,
		const FName NodeId,
		const FName EdGraphNodeTitle,
		const FName EdGraphNodeName)
		: TerminalGraphRef(TerminalGraphRef)
		, NodeId(NodeId)
		, EdGraphNodeTitle(EdGraphNodeTitle)
		, EdGraphNodeName(EdGraphNodeName)
	{
	}

#if WITH_EDITOR
	UEdGraphNode* GetGraphNode_EditorOnly() const;
#endif

	bool IsDeleted() const;
	void AppendString(FWideStringBuilderBase& Out) const;
	bool NetSerialize(FArchive& Ar, UPackageMap& Map);
	FVoxelGraphNodeRef WithSuffix(const FString& Suffix) const;
	TSharedRef<FVoxelMessageToken> CreateMessageToken() const;
	// Will return parent graph if this node is an output
	const UVoxelTerminalGraph* GetNodeTerminalGraph(const FOnVoxelGraphChanged& OnChanged) const;

public:
	FORCEINLINE bool IsExplicitlyNull() const
	{
		return
			TerminalGraphRef.IsExplicitlyNull() &&
			*this == FVoxelGraphNodeRef();
	}

	FORCEINLINE bool operator==(const FVoxelGraphNodeRef& Other) const
	{
		return
			TerminalGraphRef == Other.TerminalGraphRef &&
			NodeId == Other.NodeId &&
			TemplateInstance == Other.TemplateInstance;
	}
	FORCEINLINE bool operator!=(const FVoxelGraphNodeRef& Other) const
	{
		return
			TerminalGraphRef != Other.TerminalGraphRef ||
			NodeId != Other.NodeId ||
			TemplateInstance != Other.TemplateInstance;
	}

	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, FVoxelGraphNodeRef& Key)
	{
		Ar << Key.TerminalGraphRef;
		Ar << Key.NodeId;
		Ar << Key.TemplateInstance;
		Ar << Key.EdGraphNodeTitle;
		Ar << Key.EdGraphNodeName;
		return Ar;
	}
	FORCEINLINE friend uint32 GetTypeHash(const FVoxelGraphNodeRef& Key)
	{
		return FVoxelUtilities::MurmurHashMulti(
			GetTypeHash(Key.TerminalGraphRef),
			GetTypeHash(Key.NodeId),
			GetTypeHash(Key.TemplateInstance));
	}
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelGraphPinRef
{
	GENERATED_BODY()

	UPROPERTY()
	FVoxelGraphNodeRef NodeRef;

	UPROPERTY()
	FName PinName;

	FString ToString() const;
	const UVoxelTerminalGraph* GetNodeTerminalGraph(const FOnVoxelGraphChanged& OnChanged) const;
	TSharedRef<FVoxelMessageToken> CreateMessageToken() const;

	FORCEINLINE bool operator==(const FVoxelGraphPinRef& Other) const
	{
		return
			NodeRef == Other.NodeRef &&
			PinName == Other.PinName;
	}
	FORCEINLINE bool operator!=(const FVoxelGraphPinRef& Other) const
	{
		return
			NodeRef != Other.NodeRef ||
			PinName != Other.PinName;
	}

	FORCEINLINE friend uint32 GetTypeHash(const FVoxelGraphPinRef& Key)
	{
		return FVoxelUtilities::MurmurHashMulti(
			GetTypeHash(Key.NodeRef),
			GetTypeHash(Key.PinName));
	}
};