// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelGraphNodeRef.h"
#include "VoxelSerializedGraph.h"
#include "VoxelTerminalGraphRuntime.generated.h"

namespace Voxel::Graph
{
	class FGraph;
}
class FVoxelGraphEvaluator;

#if WITH_EDITOR
class IVoxelGraphEditorInterface
{
public:
	IVoxelGraphEditorInterface() = default;
	virtual ~IVoxelGraphEditorInterface() = default;

	virtual void CompileAll() = 0;
	virtual void ReconstructAllNodes(UVoxelTerminalGraph& TerminalGraph) = 0;
	virtual bool HasNode(const UVoxelTerminalGraph& TerminalGraph, const UScriptStruct* Struct) = 0;
	virtual UEdGraph* CreateEdGraph(UVoxelTerminalGraph& TerminalGraph) = 0;
	// Should be free of side effect (outside of preview fixups & migrations), ie no messages
	virtual FVoxelSerializedGraph Translate(UVoxelTerminalGraph& TerminalGraph) = 0;
};

extern VOXELGRAPHCORE_API IVoxelGraphEditorInterface* GVoxelGraphEditorInterface;
#endif

UCLASS(Within=VoxelTerminalGraph)
class VOXELGRAPHCORE_API UVoxelTerminalGraphRuntime : public UObject
{
	GENERATED_BODY()

public:
	const UVoxelGraph& GetGraph() const;
	UVoxelTerminalGraph& GetTerminalGraph();
	const UVoxelTerminalGraph& GetTerminalGraph() const;

	const FVoxelSerializedGraph& GetSerializedGraph() const;
	void EnsureIsCompiled();
	TSharedPtr<FVoxelGraphEvaluator> CreateEvaluator(const FVoxelGraphPinRef& GraphPinRef);
#if WITH_EDITOR
	void BindOnEdGraphChanged();
#endif

public:
	template<typename T>
	bool HasNode() const
	{
		return this->HasNode(StaticStructFast<T>());
	}
	bool HasNode(const UScriptStruct* Struct) const;

public:
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);

	//~ Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface

public:
	FSimpleMulticastDelegate OnMessagesChanged;

	const TVoxelArray<TSharedRef<FVoxelMessage>>& GetRuntimeMessages() const
	{
		return RuntimeMessages;
	}
	const TVoxelArray<TSharedRef<FVoxelMessage>>& GetCompileMessages() const
	{
		return CompileMessages;
	}
	const TMap<FName, TVoxelArray<TSharedRef<FVoxelMessage>>>& GetPinNameToCompileMessages() const
	{
		return PinNameToCompileMessages;
	}

	void AddMessage(const TSharedRef<FVoxelMessage>& Message);
	bool HasCompileMessages() const;

private:
	// Runtime messages that referenced this graph
	TVoxelArray<TSharedRef<FVoxelMessage>> RuntimeMessages;
	// Compile errors for the entire graph
	TVoxelArray<TSharedRef<FVoxelMessage>> CompileMessages;
	// Compile errors for a specific pin ref
	TMap<FName, TVoxelArray<TSharedRef<FVoxelMessage>>> PinNameToCompileMessages;

	UPROPERTY(DuplicateTransient, NonTransactional)
	FVoxelSerializedGraph PrivateSerializedGraph;

#if WITH_EDITOR
	void Translate();
#endif

private:
#if WITH_EDITOR
	FSharedVoidPtr OnEdGraphChangedPtr;
	FSharedVoidPtr OnTranslatedPtr;
#endif
	TOptional<TSharedPtr<const Voxel::Graph::FGraph>> CachedCompiledGraph;

	TSharedPtr<const Voxel::Graph::FGraph> Compile();

	TSharedPtr<FVoxelGraphEvaluator> CreateEvaluatorImpl(
		FName NodeId,
		FName PinName,
		const Voxel::Graph::FGraph& CompiledGraph);

	static TSharedPtr<const Voxel::Graph::FGraph> Compile(
		const FOnVoxelGraphChanged& OnTranslated,
		const UVoxelTerminalGraph& TerminalGraph,
		bool bEnableLogging,
		TVoxelArray<TSharedRef<FVoxelMessage>>& OutCompileMessages);
};