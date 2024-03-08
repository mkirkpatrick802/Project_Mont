// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelPinType.h"

struct FVoxelNode;
struct FVoxelGraphToolkit;
class UVoxelTerminalGraph;

class FVoxelGraphContextActionsBuilder
{
public:
	static void Build(FGraphContextMenuBuilder& MenuBuilder);

private:
	static constexpr int32 GroupId_Functions = 0;
	static constexpr int32 GroupId_Graphs = 0;
	static constexpr int32 GroupId_Operators = 0;
	static constexpr int32 GroupId_StructNodes = 0;
	static constexpr int32 GroupId_ShortList = 1;
	static constexpr int32 GroupId_InlineFunctions = 2;
	static constexpr int32 GroupId_Parameters = 2;

	FGraphContextMenuBuilder& MenuBuilder;
	FVoxelGraphToolkit& Toolkit;
	UVoxelTerminalGraph& ActiveTerminalGraph;

	TArray<TSharedPtr<FEdGraphSchemaAction>> ShortListActions_Forced;
	TArray<TSharedPtr<FEdGraphSchemaAction>> ShortListActions_ExactMatches;
	TArray<TSharedPtr<FEdGraphSchemaAction>> ShortListActions_Parameters;

	FVoxelGraphContextActionsBuilder(
		FGraphContextMenuBuilder& MenuBuilder,
		FVoxelGraphToolkit& Toolkit,
		UVoxelTerminalGraph& ActiveTerminalGraph)
		: MenuBuilder(MenuBuilder)
		, Toolkit(Toolkit)
		, ActiveTerminalGraph(ActiveTerminalGraph)
	{
	}

	void Build();
	void AddShortListActions();
	bool CanCallTerminalGraph(const UVoxelTerminalGraph* TerminalGraph) const;

private:
	template<typename T>
	static TSharedRef<T> MakeShortListAction(const TSharedRef<T>& Action)
	{
		const TSharedRef<T> ShortListAction = MakeSharedCopy(*Action);
		ShortListAction->Grouping = GroupId_ShortList;
		ShortListAction->UpdateSearchData(
			Action->GetMenuDescription(),
			Action->GetTooltipDescription(),
			{},
			Action->GetKeywords());
		return ShortListAction;
	}

	static TMap<FVoxelPinType, TSet<FVoxelPinType>> CollectOperatorPermutations(
		const FVoxelNode& Node,
		const UEdGraphPin& FromPin,
		const FVoxelPinTypeSet& PromotionTypes);
};