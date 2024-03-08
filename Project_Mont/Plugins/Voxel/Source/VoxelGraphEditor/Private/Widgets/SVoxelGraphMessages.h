// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraphNodeRef.h"

class UVoxelGraph;

class SVoxelGraphMessages : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(UVoxelGraph*, Graph);
	};

	void Construct(const FArguments& Args);
	void UpdateNodes();

private:
	class FGraphNode;
	class FPinNode;
	class FMessageNode;

	class INode
	{
	public:
		INode() = default;
		virtual ~INode() = default;

		virtual TSharedRef<SWidget> GetWidget() const = 0;
		virtual TArray<TSharedPtr<INode>> GetChildren() const = 0;
	};
	class FRootNode : public INode
	{
	public:
		const FString Text;
		TMap<TWeakObjectPtr<UVoxelTerminalGraph>, TSharedPtr<FGraphNode>> GraphToNode;

		explicit FRootNode(const FString& Text)
			: Text(Text)
		{
		}

		//~ Begin INode Interface
		virtual TSharedRef<SWidget> GetWidget() const override;
		virtual TArray<TSharedPtr<INode>> GetChildren() const override;
		//~ End INode Interface
	};
	class FGraphNode : public INode
	{
	public:
		const TWeakObjectPtr<UVoxelTerminalGraph> Graph;
		TMap<FName, TSharedPtr<FPinNode>> PinNameToNode;
		TMap<TSharedPtr<FVoxelMessage>, TSharedPtr<FMessageNode>> MessageToNode;

		explicit FGraphNode(const TWeakObjectPtr<UVoxelTerminalGraph>& Graph)
			: Graph(Graph)
		{
		}

		//~ Begin INode Interface
		virtual TSharedRef<SWidget> GetWidget() const override;
		virtual TArray<TSharedPtr<INode>> GetChildren() const override;
		//~ End INode Interface
	};
	class FPinNode : public INode
	{
	public:
		const FName PinName;
		TMap<TSharedPtr<FVoxelMessage>, TSharedPtr<FMessageNode>> MessageToNode;

		explicit FPinNode(const FName PinName)
			: PinName(PinName)
		{
		}

		//~ Begin INode Interface
		virtual TSharedRef<SWidget> GetWidget() const override;
		virtual TArray<TSharedPtr<INode>> GetChildren() const override;
		//~ End INode Interface
	};
	class FMessageNode : public INode
	{
	public:
		const TSharedRef<FVoxelMessage> Message;

		explicit FMessageNode(const TSharedRef<FVoxelMessage>& Message)
			: Message(Message)
		{
		}

		//~ Begin INode Interface
		virtual TSharedRef<SWidget> GetWidget() const override;
		virtual TArray<TSharedPtr<INode>> GetChildren() const override;
		//~ End INode Interface
	};

private:
	using STree = STreeView<TSharedPtr<INode>>;

	TWeakObjectPtr<UVoxelGraph> WeakGraph;
	TSharedPtr<STree> Tree;
	TArray<TSharedPtr<INode>> Nodes;

	TVoxelMap<TWeakPtr<FVoxelMessage>, TWeakPtr<FVoxelMessage>> MessageToCanonMessage;
	TMap<uint64, TWeakPtr<FVoxelMessage>> HashToCanonMessage;

	const TSharedRef<FRootNode> CompileNode = MakeVoxelShared<FRootNode>("Compile Errors");
	const TSharedRef<FRootNode> RuntimeNode = MakeVoxelShared<FRootNode>("Runtime Errors");

	TSharedRef<FVoxelMessage> GetCanonMessage(const TSharedRef<FVoxelMessage>& Message);
};