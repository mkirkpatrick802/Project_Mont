// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphSchema.h"
#include "VoxelNodeLibrary.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphVisuals.h"
#include "VoxelTerminalGraph.h"
#include "VoxelGraphSchemaAction.h"
#include "VoxelGraphContextMenuBuilder.h"
#include "VoxelGraphContextActionsBuilder.h"
#include "Nodes/VoxelGraphNode.h"
#include "Nodes/VoxelGraphNode_Knot.h"

bool UVoxelGraphSchema::ConnectionCausesLoop(const UEdGraphPin* InputPin, const UEdGraphPin* OutputPin) const
{
	const UEdGraphNode* StartNode = OutputPin->GetOwningNode();

	TSet<UEdGraphNode*> ProcessedNodes;

	TArray<UEdGraphNode*> NodesToProcess;
	NodesToProcess.Add(InputPin->GetOwningNode());

	while (NodesToProcess.Num() > 0)
	{
		UEdGraphNode* Node = NodesToProcess.Pop(false);
		if (ProcessedNodes.Contains(Node))
		{
			continue;
		}
		ProcessedNodes.Add(Node);

		for (const UEdGraphPin* Pin : Node->GetAllPins())
		{
			if (Pin->Direction != EGPD_Output)
			{
				continue;
			}

			for (const UEdGraphPin* LinkedToPin : Pin->LinkedTo)
			{
				check(LinkedToPin->Direction == EGPD_Input);

				UEdGraphNode* NewNode = LinkedToPin->GetOwningNode();
				ensure(NewNode);

				if (StartNode == NewNode)
				{
					return true;
				}
				NodesToProcess.Add(NewNode);
			}
		}
	}

	return false;
}

bool UVoxelGraphSchema::CanCreateAutomaticConversionNode(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;
	if (!CategorizePinsByDirection(PinA, PinB, InputPin, OutputPin))
	{
		return false;
	}

	return FindCastAction(OutputPin->PinType, InputPin->PinType).IsValid();
}

TSharedPtr<FEdGraphSchemaAction> UVoxelGraphSchema::FindCastAction(const FEdGraphPinType& From, const FEdGraphPinType& To) const
{
	const TSharedPtr<const FVoxelNode> CastNode = FVoxelNodeLibrary::FindCastNode(From, To);
	if (!CastNode)
	{
		return nullptr;
	}

	const TSharedRef<FVoxelGraphSchemaAction_NewStructNode> Action = MakeVoxelShared<FVoxelGraphSchemaAction_NewStructNode>();
	Action->Node = CastNode;
	return Action;
}

bool UVoxelGraphSchema::TryGetPromotionType(
	const UEdGraphPin& Pin,
	const FVoxelPinType& TargetType,
	FVoxelPinType& OutType,
	FString& OutAdditionalText)
{
	OutType = {};

	const UVoxelGraphNode* Node = Cast<UVoxelGraphNode>(Pin.GetOwningNode());
	if (!ensure(Node))
	{
		return false;
	}

	FVoxelPinTypeSet Types;
	if (!Node->CanPromotePin(Pin, Types))
	{
		return false;
	}

	OutAdditionalText = Node->GetPinPromotionWarning(Pin, TargetType);

	const FVoxelPinType CurrentType(Pin.PinType);

	const auto TryType = [&](const FVoxelPinType& NewType)
		{
			if (!OutType.IsValid() &&
				Types.Contains(NewType))
			{
				OutType = NewType;
			}
		};

	if (Pin.Direction == EGPD_Input)
	{
		// We're an input pin, we can implicitly promote to buffer

		if (CurrentType.IsBuffer())
		{
			// If we're currently a buffer, try to preserve that
			TryType(TargetType.GetBufferType());
			TryType(TargetType);
		}
		else
		{
			// Otherwise try the raw type first
			TryType(TargetType);
			TryType(TargetType.GetBufferType());
		}
	}
	else
	{
		// We're an output pin, we can never implicitly promote
		TryType(TargetType);
	}

	return OutType.IsValid();
}

bool UVoxelGraphSchema::CreatePromotedConnectionSafe(UEdGraphPin*& PinA, UEdGraphPin*& PinB) const
{
	const auto TryPromote = [&](UEdGraphPin& PinToPromote, const UEdGraphPin& OtherPin)
	{
		FString AdditionalText;
		FVoxelPinType Type;
		if (!TryGetPromotionType(PinToPromote, OtherPin.PinType, Type, AdditionalText))
		{
			return false;
		}

		UVoxelGraphNode* Node = CastChecked<UVoxelGraphNode>(PinToPromote.GetOwningNode());

		const FName PinAName = PinA->GetFName();
		const FName PinBName = PinB->GetFName();
		const UVoxelGraphNode* NodeA = CastChecked<UVoxelGraphNode>(PinA->GetOwningNode());
		const UVoxelGraphNode* NodeB = CastChecked<UVoxelGraphNode>(PinB->GetOwningNode());
		{
			// Tricky: PromotePin might reconstruct the node, invalidating pin pointers
			Node->PromotePin(PinToPromote, Type);
		}
		PinA = NodeA->FindPin(PinAName);
		PinB = NodeB->FindPin(PinBName);

		return true;
	};

	if (!TryPromote(*PinB, *PinA) &&
		!TryPromote(*PinA, *PinB))
	{
		return false;
	}

	if (!ensure(PinA) ||
		!ensure(PinB) ||
		!ensure(CanCreateConnection(PinA, PinB).Response != CONNECT_RESPONSE_MAKE_WITH_PROMOTION) ||
		!ensure(TryCreateConnection(PinA, PinB)))
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	FVoxelGraphContextActionsBuilder::Build(ContextMenuBuilder);
}

bool UVoxelGraphSchema::CreateAutomaticConversionNodeAndConnections(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	UEdGraphPin* InputPin = nullptr;
	UEdGraphPin* OutputPin = nullptr;
	if (!CategorizePinsByDirection(PinA, PinB, InputPin, OutputPin))
	{
		return false;
	}

	const TSharedPtr<FEdGraphSchemaAction> CastAction = FindCastAction(OutputPin->PinType, InputPin->PinType);
	if (!CastAction)
	{
		return false;
	}

	const FVoxelTransaction Transaction(InputPin, "Create new graph node");

	FVector2D Position;
	Position.X = (InputPin->GetOwningNode()->NodePosX + OutputPin->GetOwningNode()->NodePosX) / 2.;
	Position.Y = (InputPin->GetOwningNode()->NodePosY + OutputPin->GetOwningNode()->NodePosY) / 2.;

	const UEdGraphNode* GraphNode = CastAction->PerformAction(PinA->GetOwningNode()->GetGraph(), nullptr, Position);
	const UVoxelGraphNode* Node = CastChecked<UVoxelGraphNode>(GraphNode);

	TryCreateConnection(OutputPin, Node->GetInputPin(0));
	TryCreateConnection(InputPin, Node->GetOutputPin(0));

	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FPinConnectionResponse UVoxelGraphSchema::CanCreateConnection(const UEdGraphPin* PinA, const UEdGraphPin* PinB) const
{
	// Make sure the pins are not on the same node
	if (PinA->GetOwningNode() == PinB->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "Both are on the same node");
	}

	if (PinA->bOrphanedPin ||
		PinB->bOrphanedPin)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "Cannot make new connections to orphaned pin");
	}

	if (PinA->bNotConnectable ||
		PinB->bNotConnectable)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "Pins are not connectable");
	}

	// Compare the directions
	const UEdGraphPin* InputPin = nullptr;
	const UEdGraphPin* OutputPin = nullptr;

	if (!CategorizePinsByDirection(PinA, PinB, InputPin, OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "Directions are not compatible");
	}

	check(InputPin);
	check(OutputPin);

	if (ConnectionCausesLoop(InputPin, OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "Connection would cause loop");
	}

	if (FVoxelPinType(OutputPin->PinType).CanBeCastedTo_Schema(InputPin->PinType))
	{
		// Can connect directly
		if (InputPin->LinkedTo.Num() > 0)
		{
			ECanCreateConnectionResponse Response;
			if (InputPin == PinA)
			{
				Response = CONNECT_RESPONSE_BREAK_OTHERS_A;
			}
			else
			{
				Response = CONNECT_RESPONSE_BREAK_OTHERS_B;
			}
			return FPinConnectionResponse(Response, "Replace existing connections");
		}

		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, "");
	}

	const auto TryConnect = [this](const UEdGraphPin& PinToPromote, const UEdGraphPin& OtherPin)
	{
		FString AdditionalText;
		FVoxelPinType Type;
		if (!TryGetPromotionType(PinToPromote, OtherPin.PinType, Type, AdditionalText))
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, "");
		}

		if (!AdditionalText.IsEmpty())
		{
			AdditionalText = "\n" + AdditionalText;
		}

		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_PROMOTION,
			FString::Printf(TEXT("Convert pin %s from %s to %s%s"),
				*PinToPromote.GetDisplayName().ToString(),
				*FVoxelPinType(PinToPromote.PinType).ToString(),
				*Type.ToString(),
				*AdditionalText));
	};

	{
		const FPinConnectionResponse Response = TryConnect(*PinB, *PinA);
		if (Response.Response != CONNECT_RESPONSE_DISALLOW)
		{
			return Response;
		}
	}

	if (CanCreateAutomaticConversionNode(InputPin, OutputPin))
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE, FString::Printf(TEXT("Convert %s to %s"),
			*FVoxelPinType(OutputPin->PinType).ToString(),
			*FVoxelPinType(InputPin->PinType).ToString()));
	}

	{
		const FPinConnectionResponse Response = TryConnect(*PinA, *PinB);
		if (Response.Response != CONNECT_RESPONSE_DISALLOW)
		{
			return Response;
		}
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, FString::Printf(TEXT("Cannot convert %s to %s"),
		*FVoxelPinType(OutputPin->PinType).ToString(),
		*FVoxelPinType(InputPin->PinType).ToString()));
}

bool UVoxelGraphSchema::TryCreateConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	switch (CanCreateConnection(PinA, PinB).Response)
	{
	default:
	{
		ensure(false);
		return false;
	}
	case CONNECT_RESPONSE_MAKE:
	{
		PinA->Modify();
		PinB->Modify();
		PinA->MakeLinkTo(PinB);
	}
	break;
	case CONNECT_RESPONSE_BREAK_OTHERS_A:
	{
		PinA->Modify();
		PinB->Modify();
		PinA->BreakAllPinLinks(true);
		PinA->MakeLinkTo(PinB);
	}
	break;
	case CONNECT_RESPONSE_BREAK_OTHERS_B:
	{
		PinA->Modify();
		PinB->Modify();
		PinB->BreakAllPinLinks(true);
		PinA->MakeLinkTo(PinB);
	}
	break;
	case CONNECT_RESPONSE_BREAK_OTHERS_AB:
	{
		PinA->Modify();
		PinB->Modify();
		PinA->BreakAllPinLinks(true);
		PinB->BreakAllPinLinks(true);
		PinA->MakeLinkTo(PinB);
	}
	break;
	case CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE:
	{
		if (!CreateAutomaticConversionNodeAndConnections(PinA, PinB))
		{
			return false;
		}
	}
	break;
	case CONNECT_RESPONSE_MAKE_WITH_PROMOTION:
	{
		if (!CreatePromotedConnectionSafe(PinA, PinB))
		{
			return false;
		}
	}
	break;
	case CONNECT_RESPONSE_DISALLOW:
	{
		return false;
	}
	}

	PinA->GetOwningNode()->PinConnectionListChanged(PinA);
	PinB->GetOwningNode()->PinConnectionListChanged(PinB);

	return true;
}

bool UVoxelGraphSchema::CreatePromotedConnection(UEdGraphPin* PinA, UEdGraphPin* PinB) const
{
	// CreatePromotedConnectionSafe should be used
	ensure(false);
	return false;
}

bool UVoxelGraphSchema::DoesDefaultValueMatchAutogenerated(const UEdGraphPin& InPin) const
{
	if (Super::DoesDefaultValueMatchAutogenerated(InPin))
	{
		return true;
	}

	if (!FVoxelPinType(InPin.PinType).HasPinDefaultValue())
	{
		return true;
	}

	const FVoxelPinValue Value = FVoxelPinValue::MakeFromPinDefaultValue(InPin);
	if (!Value.IsObject())
	{
		return false;
	}

	if (!Value.GetObject() &&
		!FSoftObjectPtr(InPin.AutogeneratedDefaultValue).LoadSynchronous())
	{
		// Both null, AutogeneratedDefaultValue was pointing to a now deleted asset
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<FEdGraphSchemaAction> UVoxelGraphSchema::GetCreateCommentAction() const
{
	return MakeVoxelShared<FVoxelGraphSchemaAction_NewComment>();
}

int32 UVoxelGraphSchema::GetNodeSelectionCount(const UEdGraph* Graph) const
{
	const TSharedPtr<SGraphEditor> GraphEditor = FVoxelGraphToolkit::Get(Graph)->GetActiveGraphEditor();
	if (!ensure(GraphEditor))
	{
		return 0;
	}

	return GraphEditor->GetNumberOfSelectedNodes();
}

void UVoxelGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!ensure(Menu) ||
		!ensure(Context))
	{
		return;
	}

	FVoxelGraphContextMenuBuilder::Build(*Menu, *Context);
}

void UVoxelGraphSchema::ResetPinToAutogeneratedDefaultValue(UEdGraphPin* Pin, const bool bCallModifyCallbacks) const
{
	const FVoxelPinType Type(Pin->PinType);

	const FVoxelTransaction Transaction(Pin, "Reset pin to default");

	// Type will be invalid when migrating orphaned pins
	if (!Type.IsValid() ||
		!Type.HasPinDefaultValue())
	{
		Pin->ResetDefaultValue();
		ensure(Pin->AutogeneratedDefaultValue.IsEmpty());
	}
	else if (Type.GetPinDefaultValueType().IsObject())
	{
		Pin->DefaultValue = {};

		if (Pin->AutogeneratedDefaultValue.IsEmpty())
		{
			Pin->DefaultObject = nullptr;
		}
		else
		{
			Pin->DefaultObject = LoadObject<UObject>(nullptr, *Pin->AutogeneratedDefaultValue);
			ensure(Pin->DefaultObject);
		}
	}
	else
	{
		Pin->DefaultValue = Pin->AutogeneratedDefaultValue;
		Pin->DefaultObject = nullptr;
	}

	if (bCallModifyCallbacks)
	{
		Pin->GetOwningNode()->PinDefaultValueChanged(Pin);
	}
}

void UVoxelGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
}

void UVoxelGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& GraphPosition) const
{
	const FVoxelTransaction Transaction(PinA, "Create Reroute Node");

	const FVector2D NodeSpacerSize(42.0f, 24.0f);
	const FVector2D KnotTopLeft = GraphPosition - (NodeSpacerSize * 0.5f);

	UEdGraph* ParentGraph = PinA->GetOwningNode()->GetGraph();

	UVoxelGraphNode_Knot* NewKnot = CastChecked<UVoxelGraphNode_Knot>(FVoxelGraphSchemaAction_NewKnotNode().PerformAction(
		ParentGraph,
		nullptr,
		KnotTopLeft,
		true));

	// Move the connections across (only notifying the knot, as the other two didn't really change)
	PinA->BreakLinkTo(PinB);

	PinA->MakeLinkTo(NewKnot->GetPin(PinB->Direction));
	PinB->MakeLinkTo(NewKnot->GetPin(PinA->Direction));

	NewKnot->PropagatePinType();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
	const FVoxelTransaction Transaction(&TargetNode, "Break node Links");
	Super::BreakNodeLinks(TargetNode);
}

void UVoxelGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, const bool bSendsNodeNotification) const
{
	const FVoxelTransaction Transaction(&TargetPin, "Break Pin Links");
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphSchema::TrySetDefaultValue(UEdGraphPin& Pin, const FString& NewDefaultValue, const bool bMarkAsModified) const
{
	ensure(!Pin.DefaultObject);
	Super::TrySetDefaultValue(Pin, NewDefaultValue, bMarkAsModified);
}

void UVoxelGraphSchema::TrySetDefaultObject(UEdGraphPin& Pin, UObject* NewDefaultObject, const bool bMarkAsModified) const
{
	Pin.DefaultValue = {};
	Super::TrySetDefaultObject(Pin, NewDefaultObject, bMarkAsModified);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphSchema::GetAssetsGraphHoverMessage(const TArray<FAssetData>& Assets, const UEdGraph* HoverGraph, FString& OutTooltipText, bool& OutOkIcon) const
{
	for (const FAssetData& Asset : Assets)
	{
		if (!Asset.GetClass()->IsChildOf<UVoxelGraph>())
		{
			OutOkIcon = false;
			return;
		}
	}

	OutOkIcon = true;
}

void UVoxelGraphSchema::DroppedAssetsOnGraph(const TArray<FAssetData>& Assets, const FVector2D& GraphPosition, UEdGraph* EdGraph) const
{
	for (const FAssetData& Asset : Assets)
	{
		UVoxelGraph* Graph = Cast<UVoxelGraph>(Asset.GetAsset());
		if (!Graph)
		{
			continue;
		}

		FVoxelGraphSchemaAction_NewCallGraphNode Action;
		Action.Graph = Graph;
		Action.PerformAction(EdGraph, nullptr, GraphPosition, true);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	if (!Pin)
	{
		return Super::ShouldHidePinDefaultValue(Pin);
	}

	const UVoxelGraphNode* VoxelNode = Cast<UVoxelGraphNode>(Pin->GetOwningNode());
	if (!VoxelNode)
	{
		return false;
	}

	return
		// Hide optional pins default values
		VoxelNode->IsPinOptional(*Pin) ||
		VoxelNode->ShouldHidePinDefaultValue(*Pin);
}

FLinearColor UVoxelGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FVoxelGraphVisuals::GetPinColor(PinType);
}