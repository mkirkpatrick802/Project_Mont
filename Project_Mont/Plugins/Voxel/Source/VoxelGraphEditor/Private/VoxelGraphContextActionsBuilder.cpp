// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphContextActionsBuilder.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphSchemaAction.h"
#include "VoxelFunctionLibraryAsset.h"
#include "VoxelNodeLibrary.h"
#include "VoxelTerminalGraph.h"
#include "Nodes/VoxelGraphNode.h"
#include "Nodes/VoxelOperatorNodes.h"

void FVoxelGraphContextActionsBuilder::Build(FGraphContextMenuBuilder& MenuBuilder)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(MenuBuilder.CurrentGraph))
	{
		return;
	}

	const TSharedPtr<FVoxelGraphToolkit> Toolkit = FVoxelGraphToolkit::Get(MenuBuilder.CurrentGraph);
	UVoxelTerminalGraph* TerminalGraph = MenuBuilder.CurrentGraph->GetTypedOuter<UVoxelTerminalGraph>();

	if (!ensure(Toolkit) ||
		!ensure(TerminalGraph))
	{
		return;
	}

	FVoxelGraphContextActionsBuilder ContextActions(MenuBuilder, *Toolkit, *TerminalGraph);
	ContextActions.Build();
	ContextActions.AddShortListActions();
}

void FVoxelGraphContextActionsBuilder::Build()
{
	VOXEL_FUNCTION_COUNTER();

	// Add comment
	if (!MenuBuilder.FromPin)
	{
		bool bCreateFromSelection = false;
		if (MenuBuilder.CurrentGraph)
		{
			const TSet<UEdGraphNode*> SelectedNodes = Toolkit.GetSelectedNodes();

			for (const auto* Node : SelectedNodes)
			{
				if (Node->IsA<UVoxelGraphNode>())
				{
					bCreateFromSelection = true;
					break;
				}
			}
		}

		MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_NewComment>(
			FText(),
			bCreateFromSelection ? INVTEXT("Create Comment from Selection") : INVTEXT("Add Comment"),
			INVTEXT("Creates a comment"),
			0));
	}

	// Paste here
	if (!MenuBuilder.FromPin &&
		Toolkit.GetCommands()->CanExecuteAction(FGenericCommands::Get().Paste.ToSharedRef()))
	{
		MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_Paste>(
			FText(),
			INVTEXT("Paste here"),
			FText(),
			0));
	}

	// Add reroute node
	MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_NewKnotNode>(
		FText(),
		INVTEXT("Add reroute node"),
		INVTEXT("Create a reroute node"),
		0));

	// Add call functions
	ForEachAssetOfClass<UVoxelFunctionLibraryAsset>([&](UVoxelFunctionLibraryAsset& FunctionLibrary)
	{
		for (const FGuid& Guid : FunctionLibrary.GetGraph().GetTerminalGraphs())
		{
			const UVoxelTerminalGraph* TerminalGraph = FunctionLibrary.GetGraph().FindTerminalGraph(Guid);
			if (!ensure(TerminalGraph) ||
				TerminalGraph->IsMainTerminalGraph() ||
				!TerminalGraph->bExposeToLibrary ||
				!CanCallTerminalGraph(TerminalGraph))
			{
				continue;
			}
			const FVoxelGraphMetadata Metadata = TerminalGraph->GetMetadata();

			const TSharedRef<FVoxelGraphSchemaAction_NewCallExternalFunctionNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewCallExternalFunctionNode>(
				FText::FromString(Metadata.Category),
				FText::FromString(Metadata.DisplayName),
				FText::FromString(Metadata.Description),
				GroupId_Functions);

			Action->FunctionLibrary = &FunctionLibrary;
			Action->Guid = Guid;

			MenuBuilder.AddAction(Action);
		}
	});

	// Add call graphs
	ForEachAssetOfClass<UVoxelGraph>([&](UVoxelGraph& Graph)
	{
		ensure(!Graph.IsFunctionLibrary());

		const UVoxelTerminalGraph& TerminalGraph = Graph.GetMainTerminalGraph();
		if (!CanCallTerminalGraph(&TerminalGraph) ||
			!Graph.bShowInContextMenu)
		{
			return;
		}
		const FVoxelGraphMetadata Metadata = TerminalGraph.GetMetadata();

		const TSharedRef<FVoxelGraphSchemaAction_NewCallGraphNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewCallGraphNode>(
			FText::FromString(Metadata.Category),
			FText::FromString(Metadata.DisplayName),
			FText::FromString(Metadata.Description),
			GroupId_Graphs);

		Action->Graph = &Graph;

		MenuBuilder.AddAction(Action);
	});

	// Add member functions
	for (const FGuid& Guid : Toolkit.Asset->GetTerminalGraphs())
	{
		UVoxelTerminalGraph* TerminalGraph = Toolkit.Asset->FindTerminalGraph(Guid);
		if (!ensure(TerminalGraph) ||
			TerminalGraph->IsMainTerminalGraph() ||
			!CanCallTerminalGraph(TerminalGraph))
		{
			continue;
		}
		const FVoxelGraphMetadata Metadata = TerminalGraph->GetMetadata();

		const TSharedRef<FVoxelGraphSchemaAction_NewCallFunctionNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewCallFunctionNode>(
			FText::FromString(Metadata.Category),
			FText::FromString(Metadata.DisplayName),
			FText::FromString(Metadata.Description),
			GroupId_InlineFunctions);

		Action->Guid = Guid;

		MenuBuilder.AddAction(Action);
	}

	// Add Call Parent
	for (const FGuid& Guid : Toolkit.Asset->GetTerminalGraphs())
	{
		UVoxelTerminalGraph* TerminalGraph = Toolkit.Asset->FindTerminalGraph(Guid);
		if (!ensure(TerminalGraph) ||
			TerminalGraph->IsMainTerminalGraph() ||
			TerminalGraph->IsTopmostTerminalGraph() ||
			!CanCallTerminalGraph(TerminalGraph))
		{
			continue;
		}
		const FVoxelGraphMetadata Metadata = TerminalGraph->GetMetadata();

		const TSharedRef<FVoxelGraphSchemaAction_NewCallFunctionNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewCallFunctionNode>(
			FText::FromString(Metadata.Category),
			FText::FromString("Call Parent: " + Metadata.DisplayName),
			FText::FromString(Metadata.Description),
			GroupId_InlineFunctions);

		Action->Guid = Guid;
		Action->bCallParent = true;

		MenuBuilder.AddAction(Action);
	}

	// Call Graph Parameter
	{
		const TSharedRef<FVoxelGraphSchemaAction_NewCallGraphParameterNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewCallGraphParameterNode>(
			INVTEXT("Misc"),
			INVTEXT("Call Graph Parameter"),
			INVTEXT(""),
			GroupId_Functions);

		MenuBuilder.AddAction(Action);
	}

	// Add nodes
	for (const TSharedRef<const FVoxelNode>& Node : FVoxelNodeLibrary::GetNodes())
	{
		bool bIsExactMatch = true;
		FVoxelPinTypeSet PromotionTypes;

		const bool bHasPinMatch = !MenuBuilder.FromPin || INLINE_LAMBDA
		{
			const UEdGraphPin& FromPin = *MenuBuilder.FromPin;

			FVoxelPinTypeSet FromPinPromotionTypes;
			if (const UVoxelGraphNode* ThisNode = Cast<UVoxelGraphNode>(FromPin.GetOwningNode()))
			{
				if (!ThisNode->CanPromotePin(FromPin, FromPinPromotionTypes))
				{
					FromPinPromotionTypes.Add(FromPin.PinType);
				}
			}

			FString PinTypeAliasesString;
			if (Node->GetMetadataContainer().GetStringMetaDataHierarchical(STATIC_FNAME("PinTypeAliases"), &PinTypeAliasesString))
			{
				TArray<FString> PinTypeAliases;
				PinTypeAliasesString.ParseIntoArray(PinTypeAliases, TEXT(","));

				for (const FString& PinTypeAlias : PinTypeAliases)
				{
					FVoxelPinType Type;
					if (!ensure(FVoxelPinType::TryParse(PinTypeAlias, Type)))
					{
						continue;
					}

					if ((FromPin.Direction == EGPD_Input && Type.CanBeCastedTo_Schema(FromPin.PinType)) ||
						(FromPin.Direction == EGPD_Output && FVoxelPinType(FromPin.PinType).CanBeCastedTo_Schema(Type)))
					{
						return true;
					}
				}
			}

			for (const FVoxelPin& ToPin : Node->GetPins())
			{
				if (FromPin.Direction == (ToPin.bIsInput ? EGPD_Input : EGPD_Output))
				{
					continue;
				}

				FVoxelPinTypeSet ToPinPromotionTypes;
				if (ToPin.IsPromotable())
				{
					ToPinPromotionTypes = Node->GetPromotionTypes(ToPin);
				}
				else
				{
					ToPinPromotionTypes.Add(ToPin.GetType());
				}

				if (ToPinPromotionTypes.GetSetType() == EVoxelPinTypeSetType::All)
				{
					PromotionTypes = ToPinPromotionTypes;
					bIsExactMatch = false;
					return true;
				}

				for (const FVoxelPinType& Type : ToPinPromotionTypes.GetTypes())
				{
					for (const FVoxelPinType& FromType : FromPinPromotionTypes.GetTypes())
					{
						if (FromPin.Direction == EGPD_Input && Type.CanBeCastedTo_Schema(FromType))
						{
							PromotionTypes = ToPinPromotionTypes;
							return true;
						}
						if (FromPin.Direction == EGPD_Output && FromType.CanBeCastedTo_Schema(Type))
						{
							PromotionTypes = ToPinPromotionTypes;
							return true;
						}
					}
				}
			}

			return false;
		};

		if (!bHasPinMatch)
		{
			continue;
		}

		FString Keywords;
		Node->GetMetadataContainer().GetStringMetaDataHierarchical(STATIC_FNAME("Keywords"), &Keywords);
		Keywords += Node->GetMetadataContainer().GetMetaData(STATIC_FNAME("CompactNodeTitle"));

		if (Node->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("NativeMakeFunc")))
		{
			Keywords += "construct build";
		}
		if (Node->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("Autocast")))
		{
			Keywords += "cast convert";
		}

		FString DisplayName = Node->GetDisplayName();
		Keywords += DisplayName;
		DisplayName.ReplaceInline(TEXT("\n"), TEXT(" "));

		if (MenuBuilder.FromPin &&
			Node->GetMetadataContainer().HasMetaData(STATIC_FNAME("Operator")))
		{
			const FString Operator = Node->GetMetadataContainer().GetMetaData(STATIC_FNAME("Operator"));

			TMap<FVoxelPinType, TSet<FVoxelPinType>> Permutations = CollectOperatorPermutations(*Node, *MenuBuilder.FromPin, PromotionTypes);
			for (const auto& It : Permutations)
			{
				FVoxelPinType InnerType = It.Key.GetInnerType();
				for (const FVoxelPinType& SecondType : It.Value)
				{
					FVoxelPinType SecondInnerType = SecondType.GetInnerType();

					const TSharedRef<FVoxelGraphSchemaAction_NewPromotableStructNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewPromotableStructNode>(
						FText::FromString(Node->GetCategory()),
						FText::FromString(InnerType.ToString() + " " + Operator + " " + SecondInnerType.ToString()),
						FText::FromString(Node->GetTooltip()),
						GroupId_Operators,
						FText::FromString(Keywords + " " + InnerType.ToString() + " " + SecondInnerType.ToString()));

					Action->PinTypes = { SecondType, It.Key };
					Action->Node = Node;

					MenuBuilder.AddAction(Action);
				}
			}
		}
		else
		{
			const TSharedRef<FVoxelGraphSchemaAction_NewStructNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewStructNode>(
				FText::FromString(Node->GetCategory()),
				FText::FromString(DisplayName),
				FText::FromString(Node->GetTooltip()),
				GroupId_StructNodes,
				FText::FromString(Keywords));

			Action->Node = Node;

			MenuBuilder.AddAction(Action);

			if (Node->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("ShowInRootShortList")) ||
				(MenuBuilder.FromPin && Node->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("ShowInShortList"))))
			{
				ShortListActions_Forced.Add(MakeShortListAction(Action));
			}
			else if (MenuBuilder.FromPin && bIsExactMatch)
			{
				ShortListActions_ExactMatches.Add(MakeShortListAction(Action));
			}
		}
	}

	// Add Get Parameters
	if (!MenuBuilder.FromPin ||
		MenuBuilder.FromPin->Direction == EGPD_Input)
	{
		for (const FGuid& Guid : Toolkit.Asset->GetParameters())
		{
			const FVoxelParameter& Parameter = Toolkit.Asset->FindParameterChecked(Guid);
			if (MenuBuilder.FromPin &&
				!Parameter.Type.CanBeCastedTo_Schema(MenuBuilder.FromPin->PinType))
			{
				continue;
			}

			const TSharedRef<FVoxelGraphSchemaAction_NewParameterUsage> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewParameterUsage>(
				FText::FromString("Parameters"),
				FText::FromString("Get " + Parameter.Name.ToString()),
				FText::FromString(Parameter.Description),
				GroupId_Parameters);

			Action->Guid = Guid;
			Action->Type = Parameter.Type;

			MenuBuilder.AddAction(Action);

			if (MenuBuilder.FromPin)
			{
				ShortListActions_Parameters.Add(MakeShortListAction(Action));
			}
		}
	}

	// Add Get Inputs
	if (!MenuBuilder.FromPin ||
		MenuBuilder.FromPin->Direction == EGPD_Input)
	{
		const UVoxelTerminalGraph& MainTerminalGraph = ActiveTerminalGraph.GetGraph().GetMainTerminalGraph();

		for (const FGuid& Guid : MainTerminalGraph.GetInputs())
		{
			const FVoxelGraphInput& Input = MainTerminalGraph.FindInputChecked(Guid);
			if (MenuBuilder.FromPin &&
				!Input.Type.CanBeCastedTo_Schema(MenuBuilder.FromPin->PinType))
			{
				continue;
			}

			const TSharedRef<FVoxelGraphSchemaAction_NewGraphInputUsage> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewGraphInputUsage>(
				FText::FromString("Graph Inputs"),
				FText::FromString("Get " + Input.Name.ToString()),
				FText::FromString(Input.Description),
				GroupId_Parameters);

			Action->Guid = Guid;
			Action->Type = Input.Type;

			MenuBuilder.AddAction(Action);

			if (MenuBuilder.FromPin)
			{
				ShortListActions_Parameters.Add(MakeShortListAction(Action));
			}
		}

		if (!ActiveTerminalGraph.IsMainTerminalGraph())
		{
			for (const FGuid& Guid : ActiveTerminalGraph.GetInputs())
			{
				const FVoxelGraphInput& Input = ActiveTerminalGraph.FindInputChecked(Guid);
				if (MenuBuilder.FromPin &&
					!Input.Type.CanBeCastedTo_Schema(MenuBuilder.FromPin->PinType))
				{
					continue;
				}

				const TSharedRef<FVoxelGraphSchemaAction_NewFunctionInputUsage> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewFunctionInputUsage>(
					FText::FromString("Function Inputs"),
					FText::FromString("Get " + Input.Name.ToString()),
					FText::FromString(Input.Description),
					GroupId_Parameters);

				Action->Guid = Guid;
				Action->Type = Input.Type;

				MenuBuilder.AddAction(Action);

				if (MenuBuilder.FromPin)
				{
					ShortListActions_Parameters.Add(MakeShortListAction(Action));
				}
			}
		}
	}

	// Add Set Outputs
	if (!MenuBuilder.FromPin ||
		MenuBuilder.FromPin->Direction == EGPD_Output)
	{
		for (const FGuid& Guid : ActiveTerminalGraph.GetOutputs())
		{
			const FVoxelGraphOutput& Output = ActiveTerminalGraph.FindOutputChecked(Guid);
			if (MenuBuilder.FromPin &&
				!FVoxelPinType(MenuBuilder.FromPin->PinType).CanBeCastedTo_Schema(Output.Type))
			{
				continue;
			}

			const TSharedRef<FVoxelGraphSchemaAction_NewOutputUsage> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewOutputUsage>(
				ActiveTerminalGraph.IsMainTerminalGraph()
				? FText::FromString("Graph Outputs")
				: FText::FromString("Function Outputs"),
				FText::FromString("Set " + Output.Name.ToString()),
				FText::FromString(Output.Description),
				GroupId_Parameters);

			Action->Guid = Guid;
			Action->Type = Output.Type;

			MenuBuilder.AddAction(Action);

			if (MenuBuilder.FromPin)
			{
				ShortListActions_Parameters.Add(MakeShortListAction(Action));
			}
		}
	}

	// Add Get/Set Local Variable
	for (const FGuid& Guid : ActiveTerminalGraph.GetLocalVariables())
	{
		const FVoxelGraphLocalVariable& LocalVariable = ActiveTerminalGraph.FindLocalVariableChecked(Guid);

		for (const bool bIsDeclaration : TArray<bool>{ false, true })
		{
			if (MenuBuilder.FromPin)
			{
				if (MenuBuilder.FromPin->Direction == EGPD_Input)
				{
					if (bIsDeclaration)
					{
						continue;
					}

					if (!LocalVariable.Type.CanBeCastedTo_Schema(MenuBuilder.FromPin->PinType))
					{
						continue;
					}
				}
				else
				{
					check(MenuBuilder.FromPin->Direction == EGPD_Output);

					if (!bIsDeclaration)
					{
						continue;
					}

					if (!FVoxelPinType(MenuBuilder.FromPin->PinType).CanBeCastedTo_Schema(LocalVariable.Type))
					{
						continue;
					}
				}
			}

			const TSharedRef<FVoxelGraphSchemaAction_NewLocalVariableUsage> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewLocalVariableUsage>(
				FText::FromString("Local Variables"),
				FText::FromString((bIsDeclaration ? "Set " : "Get ") + LocalVariable.Name.ToString()),
				FText::FromString(LocalVariable.Description),
				GroupId_Parameters);

			Action->Guid = Guid;
			Action->Type = LocalVariable.Type;
			Action->bIsDeclaration = bIsDeclaration;

			MenuBuilder.AddAction(Action);

			if (MenuBuilder.FromPin)
			{
				ShortListActions_Parameters.Add(MakeShortListAction(Action));
			}
		}
	}

	// Add Promote to parameter
	if (!MenuBuilder.FromPin ||
		MenuBuilder.FromPin->Direction == EGPD_Input)
	{
		MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_NewParameter>(
			INVTEXT(""),
			FText::FromString(MenuBuilder.FromPin ? "Promote to parameter" : "Create new parameter"),
			FText::FromString(MenuBuilder.FromPin ? "Promote to parameter" : "Create new parameter"),
			GroupId_Parameters));
	}

	// Add Promote to input
	if (!MenuBuilder.FromPin ||
		MenuBuilder.FromPin->Direction == EGPD_Input)
	{
		MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_NewGraphInput>(
			INVTEXT(""),
			FText::FromString(MenuBuilder.FromPin ? "Promote to graph input" : "Create new graph input"),
			FText::FromString(MenuBuilder.FromPin ? "Promote to graph input" : "Create new graph input"),
			GroupId_Parameters));

		if (!ActiveTerminalGraph.IsMainTerminalGraph())
		{
			MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_NewFunctionInput>(
				INVTEXT(""),
				FText::FromString(MenuBuilder.FromPin ? "Promote to function input" : "Create new function input"),
				FText::FromString(MenuBuilder.FromPin ? "Promote to function input" : "Create new function input"),
				GroupId_Parameters));
		}
	}

	// Add Promote to output
	if (!MenuBuilder.FromPin ||
		MenuBuilder.FromPin->Direction == EGPD_Output)
	{
		MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_NewOutput>(
			INVTEXT(""),
			FText::FromString(MenuBuilder.FromPin ? "Promote to output" : "Create new output"),
			FText::FromString(MenuBuilder.FromPin ? "Promote to output" : "Create new output"),
			GroupId_Parameters));
	}

	// Add Promote to local variable
	MenuBuilder.AddAction(MakeVoxelShared<FVoxelGraphSchemaAction_NewLocalVariable>(
		INVTEXT(""),
		FText::FromString(MenuBuilder.FromPin ? "Promote to local variable" : "Create new local variable"),
		FText::FromString(MenuBuilder.FromPin ? "Promote to local variable" : "Create new local variable"),
		GroupId_Parameters));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphContextActionsBuilder::AddShortListActions()
{
	for (const TSharedPtr<FEdGraphSchemaAction>& Action : ShortListActions_Forced)
	{
		MenuBuilder.AddAction(Action);
	}

	if (ShortListActions_ExactMatches.Num() < 10)
	{
		for (const TSharedPtr<FEdGraphSchemaAction>& Action : ShortListActions_ExactMatches)
		{
			MenuBuilder.AddAction(Action);
		}
	}

	for (const TSharedPtr<FEdGraphSchemaAction>& Action : ShortListActions_Parameters)
	{
		MenuBuilder.AddAction(Action);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelGraphContextActionsBuilder::CanCallTerminalGraph(const UVoxelTerminalGraph* TerminalGraph) const
{
	if (!ensure(TerminalGraph))
	{
		return false;
	}

	if (!MenuBuilder.FromPin)
	{
		return true;
	}

	if (MenuBuilder.FromPin->Direction == EGPD_Input)
	{
		for (const FGuid& Guid : TerminalGraph->GetOutputs())
		{
			if (TerminalGraph->FindOutputChecked(Guid).Type.CanBeCastedTo_Schema(MenuBuilder.FromPin->PinType))
			{
				return true;
			}
		}
	}
	else
	{
		check(MenuBuilder.FromPin->Direction == EGPD_Output);

		for (const FGuid& Guid : TerminalGraph->GetInputs())
		{
			if (FVoxelPinType(MenuBuilder.FromPin->PinType).CanBeCastedTo_Schema(TerminalGraph->FindInputChecked(Guid).Type))
			{
				return true;
			}
		}
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TMap<FVoxelPinType, TSet<FVoxelPinType>> FVoxelGraphContextActionsBuilder::CollectOperatorPermutations(
	const FVoxelNode& Node,
	const UEdGraphPin& FromPin,
	const FVoxelPinTypeSet& PromotionTypes)
{
	const FVoxelPinType FromPinType = FVoxelPinType(FromPin.PinType);
	const bool bIsBuffer = FromPinType.IsBuffer();

	if (!PromotionTypes.Contains(FromPinType))
	{
		return {};
	}

	const bool bIsCommutativeOperator = Node.IsA<FVoxelTemplateNode_CommutativeAssociativeOperator>();

	TMap<FVoxelPinType, TSet<FVoxelPinType>> Result;
	if (FromPin.Direction == EGPD_Output)
	{
		for (const FVoxelPinType& Type : PromotionTypes.GetTypes())
		{
			if (Type.IsBuffer() != bIsBuffer)
			{
				continue;
			}

			Result.FindOrAdd(FromPinType, {}).Add(Type);
			if (!bIsCommutativeOperator)
			{
				Result.FindOrAdd(Type, {}).Add(FromPinType);
			}
		}
	}
	else
	{
		const int32 FromDimension = FVoxelTemplateNodeUtilities::GetDimension(FromPinType);

		for (const FVoxelPinType& Type : PromotionTypes.GetTypes())
		{
			if (Type.IsBuffer() != bIsBuffer ||
				FromDimension < FVoxelTemplateNodeUtilities::GetDimension(Type))
			{
				continue;
			}

			if (bIsCommutativeOperator)
			{
				Result.FindOrAdd(FromPinType, {}).Add(Type);
				continue;
			}

			for (const FVoxelPinType& SecondType : PromotionTypes.GetTypes())
			{
				if (SecondType.IsBuffer() != bIsBuffer ||
					FromDimension < FVoxelTemplateNodeUtilities::GetDimension(SecondType))
				{
					continue;
				}

				if (Type != FromPinType &&
					SecondType != FromPinType)
				{
					continue;
				}

				Result.FindOrAdd(Type, {}).Add(SecondType);
			}
		}
	}

	return Result;
}