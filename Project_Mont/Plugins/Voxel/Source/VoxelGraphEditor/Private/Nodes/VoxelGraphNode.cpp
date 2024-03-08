// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode.h"
#include "VoxelEdGraph.h"
#include "VoxelGraphSchema.h"
#include "VoxelNodeLibrary.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphTracker.h"
#include "Preview/VoxelPreviewHandler.h"
#include "Channel/VoxelChannelAsset_DEPRECATED.h"

bool UVoxelGraphNode::IsVisible(const IVoxelNodeDefinition::FNode& Node) const
{
	if (Node.IsCategory() &&
		Node.GetPath().Num() == 1 &&
		Node.Name == STATIC_FNAME("Advanced"))
	{
		return bShowAdvanced;
	}

	const TSet<FName>& CollapsedCategories = Node.IsInput() ? CollapsedInputCategories : CollapsedOutputCategories;
	if (CollapsedCategories.Contains(Node.GetConcatenatedPath()))
	{
		return false;
	}

	const TSharedPtr<IVoxelNodeDefinition::FNode> Parent = Node.WeakParent.Pin();
	if (!Parent)
	{
		return true;
	}
	return IsVisible(*Parent);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TArray<UEdGraphPin*> UVoxelGraphNode::GetInputPins() const
{
	return Pins.FilterByPredicate([&](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Input; });
}

TArray<UEdGraphPin*> UVoxelGraphNode::GetOutputPins() const
{
	return Pins.FilterByPredicate([&](const UEdGraphPin* Pin) { return Pin->Direction == EGPD_Output; });
}

UEdGraphPin* UVoxelGraphNode::GetInputPin(const int32 Index) const
{
	const TArray<UEdGraphPin*> InputPins = GetInputPins();
	if (!ensure(InputPins.IsValidIndex(Index)))
	{
		return nullptr;
	}
	return InputPins[Index];
}

UEdGraphPin* UVoxelGraphNode::GetOutputPin(const int32 Index) const
{
	const TArray<UEdGraphPin*> OutputPins = GetOutputPins();
	if (!ensure(OutputPins.IsValidIndex(Index)))
	{
		return nullptr;
	}
	return OutputPins[Index];
}

UEdGraphPin* UVoxelGraphNode::FindPinByPredicate_Unique(TFunctionRef<bool(UEdGraphPin* Pin)> Function) const
{
	UEdGraphPin* FoundPin = nullptr;
	for (UEdGraphPin* Pin : Pins)
	{
		if (!Function(Pin))
		{
			continue;
		}

		ensure(!FoundPin);
		FoundPin = Pin;
	}

	return FoundPin;
}

const UVoxelGraphSchema& UVoxelGraphNode::GetSchema() const
{
	return *CastChecked<const UVoxelGraphSchema>(Super::GetSchema());
}

TSharedPtr<FVoxelGraphToolkit> UVoxelGraphNode::GetToolkit() const
{
	return CastChecked<UVoxelEdGraph>(GetGraph())->GetGraphToolkit();
}

void UVoxelGraphNode::RefreshNode()
{
	const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
	if (!Toolkit)
	{
		// Asset might not be opened
		return;
	}

	const UEdGraph* EdGraph = GetGraph();
	if (!ensure(EdGraph))
	{
		return;
	}

	const TSharedPtr<SGraphEditor> GraphEditor = Toolkit->FindGraphEditor(*EdGraph);
	if (!GraphEditor)
	{
		// Graph might not be opened
		return;
	}

	GraphEditor->RefreshNode(*this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphNode::CanSplitPin(const UEdGraphPin& Pin) const
{
	if (Pin.ParentPin ||
		Pin.bOrphanedPin ||
		Pin.LinkedTo.Num() > 0 ||
		!ensure(Pin.SubPins.Num() == 0))
	{
		return false;
	}

	if (Pin.Direction == EGPD_Input)
	{
		return FVoxelNodeLibrary::FindMakeNode(Pin.PinType) != nullptr;
	}
	else
	{
		ensure(Pin.Direction == EGPD_Output);
		return FVoxelNodeLibrary::FindBreakNode(Pin.PinType) != nullptr;
	}
}

void UVoxelGraphNode::SplitPin(UEdGraphPin& Pin)
{
	Modify();

	const TSharedPtr<const FVoxelNode> NodeTemplate = Pin.Direction == EGPD_Input
			? FVoxelNodeLibrary::FindMakeNode(Pin.PinType)
			: FVoxelNodeLibrary::FindBreakNode(Pin.PinType);

	if (!ensure(NodeTemplate))
	{
		return;
	}

	const TSharedRef<FVoxelNode> Node = NodeTemplate->MakeSharedCopy();
	{
		FVoxelPin& NodePin =
			Pin.Direction == EGPD_Input
			? Node->GetUniqueOutputPin()
			: Node->GetUniqueInputPin();

		if (NodePin.IsPromotable() &&
			ensure(Node->GetPromotionTypes(NodePin).Contains(Pin.PinType)))
		{
			// Promote so the type are correct - eg if we are an array pin, the split pin should be array too
			Node->PromotePin(NodePin, Pin.PinType);
		}
	}

	TArray<const FVoxelPin*> NewPins;
	for (const FVoxelPin& NewPin : Node->GetPins())
	{
		if (Pin.Direction == EGPD_Input && !NewPin.bIsInput)
		{
			continue;
		}
		if (Pin.Direction == EGPD_Output && NewPin.bIsInput)
		{
			continue;
		}

		NewPins.Add(&NewPin);
	}

	Pin.bHidden = true;

	for (const FVoxelPin* NewPin : NewPins)
	{
		UEdGraphPin* SubPin = CreatePin(
			NewPin->bIsInput ? EGPD_Input : EGPD_Output,
			NewPin->GetType().GetEdGraphPinType(),
			NewPin->Name,
			Pins.IndexOfByKey(&Pin));

		FString Name = Pin.GetDisplayName().ToString();
		if (IsCompact())
		{
			Name.Reset();
		}
		if (!Name.IsEmpty())
		{
			Name += " ";
		}
		Name += NewPin->Metadata.DisplayName;

		SubPin->PinFriendlyName = FText::FromString(Name);
		SubPin->PinToolTip = Name + "\n" + Pin.PinToolTip;

		SubPin->ParentPin = &Pin;
		Pin.SubPins.Add(SubPin);
	}

	RefreshNode();
}

bool UVoxelGraphNode::CanRecombinePin(const UEdGraphPin& Pin) const
{
	if (!Pin.ParentPin)
	{
		return false;
	}

	ensure(Pin.ParentPin->bHidden);
	ensure(Pin.ParentPin->LinkedTo.Num() == 0);
	ensure(Pin.ParentPin->SubPins.Contains(&Pin));

	return true;
}

void UVoxelGraphNode::RecombinePin(UEdGraphPin& Pin)
{
	Modify();

	UEdGraphPin* ParentPin = Pin.ParentPin;
	check(ParentPin);

	ensure(ParentPin->bHidden);
	ParentPin->bHidden = false;

	const TArray<UEdGraphPin*> SubPins = ParentPin->SubPins;
	ensure(SubPins.Num() > 0);

	for (UEdGraphPin* SubPin : SubPins)
	{
		SubPin->MarkAsGarbage();
		ensure(Pins.Remove(SubPin) == 1);
	}

	ensure(ParentPin->SubPins.Num() == 0);

	RefreshNode();
}

bool UVoxelGraphNode::TryMigratePin(UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	if (FVoxelPinType(OldPin->PinType).Is<FVoxelChannelRef_DEPRECATED>() &&
		FVoxelPinType(NewPin->PinType).Is<FVoxelChannelName>() &&
		OldPin->LinkedTo.Num() == 0 &&
		NewPin->LinkedTo.Num() == 0)
	{
		const UVoxelChannelAsset_DEPRECATED* Channel = Cast<UVoxelChannelAsset_DEPRECATED>(OldPin->DefaultObject);
		if (!ensureVoxelSlow(Channel))
		{
			return false;
		}

		NewPin->MovePersistentDataFromOldPin(*OldPin);

		NewPin->DefaultValue = FVoxelPinValue::Make(Channel->MakeChannelName()).ExportToString();
		NewPin->DefaultObject = nullptr;

		return true;
	}

	if (!FVoxelPinType(OldPin->PinType).IsWildcard() &&
		!FVoxelPinType(NewPin->PinType).IsWildcard() &&
		!FVoxelPinType(OldPin->PinType).CanBeCastedTo(FVoxelPinType(NewPin->PinType)))
	{
		return false;
	}

	NewPin->MovePersistentDataFromOldPin(*OldPin);
	return true;
}

void UVoxelGraphNode::TryMigrateDefaultValue(const UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	if (!FVoxelPinType(OldPin->PinType).HasPinDefaultValue() ||
		!FVoxelPinType(NewPin->PinType).HasPinDefaultValue())
	{
		return;
	}

	const FVoxelPinValue OldValue = FVoxelPinValue::MakeFromPinDefaultValue(*OldPin);

	FVoxelPinValue NewValue(FVoxelPinType(NewPin->PinType).GetPinDefaultValueType());
	if (!NewValue.ImportFromUnrelated(OldValue))
	{
		return;
	}

	NewPin->DefaultValue = NewValue.ExportToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString UVoxelGraphNode::GetSearchTerms() const
{
	return GetNodeTitle(ENodeTitleType::FullTitle).ToString().Replace(TEXT("\n"), TEXT(" "));
}

TSharedRef<IVoxelNodeDefinition> UVoxelGraphNode::GetNodeDefinition()
{
	return MakeVoxelShared<IVoxelNodeDefinition>();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode::ReconstructNode(bool bCreateOrphans)
{
	VOXEL_FUNCTION_COUNTER();

	ON_SCOPE_EXIT
	{
		GVoxelGraphTracker->NotifyEdGraphNodeChanged(*this);
	};

	Modify();
	PreReconstructNode();

	// Sanitize links
	for (UEdGraphPin* Pin : Pins)
	{
		Pin->PinType = FVoxelPinType(Pin->PinType).GetEdGraphPinType();
		Pin->LinkedTo.Remove(nullptr);

		for (UEdGraphPin* OtherPin : Pin->LinkedTo)
		{
			if (!OtherPin->GetOwningNode()->Pins.Contains(OtherPin))
			{
				Pin->LinkedTo.Remove(OtherPin);
			}
		}
	}

	// Move the existing pins to a saved array
	const TArray<UEdGraphPin*> OldPins = Pins;
	Pins.Reset();

	// Recreate the new pins
	{
		bAllocateDefaultPinsCalled = false;
		AllocateDefaultPins();
		ensure(bAllocateDefaultPinsCalled);
	}

	for (UEdGraphPin* Pin : Pins)
	{
		// Newly allocated pins should never be orphaned
		ensure(!Pin->bOrphanedPin);
	}

	// Split new pins
	for (const UEdGraphPin* OldPin : OldPins)
	{
		if (OldPin->SubPins.Num() == 0)
		{
			continue;
		}

		UEdGraphPin* NewPin = FindPinByPredicate_Unique([&](const UEdGraphPin* Pin)
		{
			return
				Pin->PinName == OldPin->PinName &&
				Pin->PinType == OldPin->PinType &&
				Pin->Direction == OldPin->Direction;
		});
		if (!NewPin)
		{
			continue;
		}

		SplitPin(*NewPin);
	}

	TMap<UEdGraphPin*, UEdGraphPin*> NewPinsToOldPins;
	TMap<UEdGraphPin*, UEdGraphPin*> OldPinsToNewPins;

	// Map by name
	for (UEdGraphPin* OldPin : OldPins)
	{
		UEdGraphPin* NewPin = FindPinByPredicate_Unique([&](const UEdGraphPin* Pin)
		{
			if (Pin->Direction != OldPin->Direction ||
				Pin->PinName != OldPin->PinName)
			{
				return false;
			}

			if ((Pin->ParentPin != nullptr) != (OldPin->ParentPin != nullptr))
			{
				return false;
			}

			if (Pin->ParentPin &&
				OldPin->ParentPin &&
				Pin->ParentPin->PinName != OldPin->ParentPin->PinName)
			{
				return false;
			}

			return true;
		});

		if (!NewPin)
		{
			NewPin = FindPinByPredicate_Unique([&](const UEdGraphPin* Pin)
			{
				if (Pin->Direction != OldPin->Direction ||
					Pin->PinName != GetMigratedPinName(OldPin->PinName))
				{
					return false;
				}

				if ((Pin->ParentPin != nullptr) != (OldPin->ParentPin != nullptr))
				{
					return false;
				}

				if (Pin->ParentPin &&
					OldPin->ParentPin &&
					Pin->ParentPin->PinName != GetMigratedPinName(OldPin->ParentPin->PinName))
				{
					return false;
				}

				return true;
			});
		}

		if (!NewPin)
		{
			continue;
		}

		ensure(!OldPinsToNewPins.Contains(OldPin));
		ensure(!NewPinsToOldPins.Contains(NewPin));

		OldPinsToNewPins.Add(OldPin, NewPin);
		NewPinsToOldPins.Add(NewPin, OldPin);
	}

	if (!bCreateOrphans)
	{
		// Map by index if we're not creating orphans
		for (int32 Index = 0; Index < OldPins.Num(); Index++)
		{
			UEdGraphPin* OldPin = OldPins[Index];
			if (OldPinsToNewPins.Contains(OldPin) ||
				!Pins.IsValidIndex(Index))
			{
				continue;
			}

			UEdGraphPin* NewPin = Pins[Index];
			if (NewPinsToOldPins.Contains(NewPin))
			{
				continue;
			}

			ensure(!OldPinsToNewPins.Contains(OldPin));
			ensure(!NewPinsToOldPins.Contains(NewPin));

			OldPinsToNewPins.Add(OldPin, NewPin);
			NewPinsToOldPins.Add(NewPin, OldPin);
		}
	}

	TSet<UEdGraphPin*> MigratedOldPins;
	for (const auto& It : OldPinsToNewPins)
	{
		UEdGraphPin* OldPin = It.Key;
		UEdGraphPin* NewPin = It.Value;

		ensure(!OldPin->bOrphanedPin);

		if (TryMigratePin(OldPin, NewPin))
		{
			ensure(OldPin->LinkedTo.Num() == 0);
			MigratedOldPins.Add(OldPin);
			continue;
		}

		if (bCreateOrphans)
		{
			continue;
		}

		// If we're not going to create an orphan try to keep the default value
		TryMigrateDefaultValue(OldPin, NewPin);
	}

	struct FConnectionToRestore
	{
		UEdGraphPin* Pin = nullptr;
		UEdGraphPin* LinkedTo = nullptr;
		UEdGraphPin* OrphanedPin = nullptr;
	};
	TArray<FConnectionToRestore> ConnectionsToRestore;

	// Throw away the original pins
	for (UEdGraphPin* OldPin : OldPins)
	{
		const bool bCanCreateOrphan = INLINE_LAMBDA
		{
			if (!bCreateOrphans &&
				!OldPin->bOrphanedPin)
			{
				return false;
			}

			if (MigratedOldPins.Contains(OldPin))
			{
				return false;
			}

			if (OldPin->LinkedTo.Num() > 0)
			{
				return true;
			}

			return !OldPin->DoesDefaultValueMatchAutogenerated();
		};

		UEdGraphPin* OrphanedPin = nullptr;
		if (bCanCreateOrphan)
		{
			FString NewName = OldPin->PinName.ToString();
			if (!NewName.StartsWith("ORPHANED_"))
			{
				// Avoid collisions
				NewName = "ORPHANED_" + NewName;
			}

			OrphanedPin = CreatePin(OldPin->Direction, OldPin->PinType, *NewName);
			OrphanedPin->bOrphanedPin = true;
			ensure(TryMigratePin(OldPin, OrphanedPin));
			OrphanedPin->PinFriendlyName = OldPin->PinFriendlyName.IsEmpty() ? FText::FromName(OldPin->PinName) : OldPin->PinFriendlyName;
		}

		if (UEdGraphPin* NewPin = OldPinsToNewPins.FindRef(OldPin))
		{
			for (UEdGraphPin* LinkedTo : OldPin->LinkedTo)
			{
				FConnectionToRestore ConnectionToRestore;
				ConnectionToRestore.Pin = NewPin;
				ConnectionToRestore.LinkedTo = LinkedTo;
				ConnectionToRestore.OrphanedPin = OrphanedPin;
				ConnectionsToRestore.Add(ConnectionToRestore);
			}
		}

		OldPin->Modify();
		OldPin->MarkAsGarbage();
	}

	// Check bNotConnectable
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->bNotConnectable)
		{
			Pin->BreakAllPinLinks();
		}
	}

	// Sanitize default values
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->bOrphanedPin)
		{
			continue;
		}

		const FVoxelPinType Type(Pin->PinType);
		if (!Type.HasPinDefaultValue())
		{
			Pin->ResetDefaultValue();
			ensure(Pin->AutogeneratedDefaultValue.IsEmpty());
			continue;
		}

		const FVoxelPinType DefaultValueType = Type.GetPinDefaultValueType();
		if (DefaultValueType.IsObject() ||
			DefaultValueType.IsClass())
		{
			if (!Pin->DefaultValue.IsEmpty() &&
				!Pin->DefaultObject)
			{
				Pin->DefaultObject = LoadObject<UObject>(nullptr, *Pin->DefaultValue);
			}
			Pin->DefaultValue.Reset();
			continue;
		}
		Pin->DefaultObject = nullptr;

		if (!ensure(DefaultValueType.IsValid()))
		{
			Pin->DefaultValue.Reset();
			continue;
		}

		if (!FVoxelPinValue(DefaultValueType).ImportFromString(Pin->DefaultValue))
		{
			GetDefault<UVoxelGraphSchema>()->ResetPinToAutogeneratedDefaultValue(Pin, false);
		}
	}

	PostReconstructNode();

	for (const FConnectionToRestore& Connection : ConnectionsToRestore)
	{
		// Never promote nor break connections
		if (GetSchema().CanCreateConnection(Connection.Pin, Connection.LinkedTo).Response != CONNECT_RESPONSE_MAKE)
		{
			continue;
		}

		if (!ensure(GetSchema().TryCreateConnection(Connection.Pin, Connection.LinkedTo)))
		{
			continue;
		}

		if (Connection.OrphanedPin)
		{
			Connection.OrphanedPin->BreakLinkTo(Connection.LinkedTo);
		}
	}

	RefreshNode();
	FixupPreviewSettings();
}

void UVoxelGraphNode::FixupPreviewSettings()
{
	VOXEL_FUNCTION_COUNTER();

	TMap<FName, FVoxelPinPreviewSettings> NameToPreviewSettings;
	for (const FVoxelPinPreviewSettings& Settings : PreviewSettings)
	{
		ensure(!NameToPreviewSettings.Contains(Settings.PinName));
		NameToPreviewSettings.Add(Settings.PinName, Settings);
	}

	PreviewSettings.Reset();

	for (const UEdGraphPin* Pin : GetOutputPins())
	{
		if (Pin->ParentPin ||
			Pin->bOrphanedPin)
		{
			continue;
		}

		FVoxelPinPreviewSettings Settings = NameToPreviewSettings.FindRef(Pin->PinName);
		Settings.PinName = Pin->PinName;
		Settings.PinType = Pin->PinType;

		if (!Settings.PreviewHandler.IsA<FVoxelPreviewHandler>() ||
			!Settings.PreviewHandler.Get<FVoxelPreviewHandler>().SupportsType(Settings.PinType))
		{
			Settings.PreviewHandler = {};

			for (const TSharedRef<const FVoxelPreviewHandler>& Handler : FVoxelPreviewHandler::GetHandlers())
			{
				if (Handler->SupportsType(Settings.PinType))
				{
					Settings.PreviewHandler = FVoxelInstancedStruct(Handler->GetStruct());
					break;
				}
			}
		}

		PreviewSettings.Add(Settings);
	}

	if (!FindPin(PreviewedPin))
	{
		for (const FVoxelPinPreviewSettings& Settings : PreviewSettings)
		{
			PreviewedPin = Settings.PinName;
			break;
		}
	}

	if (const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit())
	{
		Toolkit->GetPreviewDetailsView().ForceRefresh();
	}
}

void UVoxelGraphNode::AllocateDefaultPins()
{
	// Can't ensure as engine might call AllocateDefaultPins
	bAllocateDefaultPinsCalled = true;

	TSet<FName> Names;
	for (const UEdGraphPin* Pin : Pins)
	{
		ensure(!Names.Contains(Pin->PinName));
		Names.Add(Pin->PinName);
	}

	InputPinCategories = {};
	OutputPinCategories = {};

	TSet<FName> ValidCategories;
	for (const UEdGraphPin* Pin : Pins)
	{
		FName PinCategory = GetPinCategory(*Pin);
		ValidCategories.Add(PinCategory);

		switch (Pin->Direction)
		{
		default: check(false);
		case EGPD_Input: InputPinCategories.Add(PinCategory); break;
		case EGPD_Output: OutputPinCategories.Add(PinCategory); break;
		}
	}

	CollapsedInputCategories = CollapsedInputCategories.Intersect(ValidCategories);
	CollapsedOutputCategories = CollapsedOutputCategories.Intersect(ValidCategories);

	FixupPreviewSettings();
}

void UVoxelGraphNode::ReconstructNode()
{
	ReconstructNode(true);
}

FLinearColor UVoxelGraphNode::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}

void UVoxelGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin)
	{
		return;
	}

	const UVoxelGraphSchema& Schema = GetSchema();

	// Check non-promotable and same type pins first
	for (UEdGraphPin* Pin : Pins)
	{
		const FPinConnectionResponse Response = Schema.CanCreateConnection(FromPin, Pin);

		if (Response.Response == CONNECT_RESPONSE_MAKE)
		{
			Schema.TryCreateConnection(FromPin, Pin);
			return;
		}

		// If the pins just differ by being buffer or not, consider them to be of the same type
		if (Response.Response == CONNECT_RESPONSE_MAKE_WITH_PROMOTION &&
			FVoxelPinType(FromPin->PinType).GetInnerType() == FVoxelPinType(Pin->PinType).GetInnerType())
		{
			Schema.TryCreateConnection(FromPin, Pin);
			return;
		}
	}

	for (UEdGraphPin* Pin : Pins)
	{
		const FPinConnectionResponse Response = Schema.CanCreateConnection(FromPin, Pin);

		if (Response.Response == CONNECT_RESPONSE_MAKE_WITH_PROMOTION)
		{
			Schema.TryCreateConnection(FromPin, Pin);
			return;
		}
		else if (Response.Response == CONNECT_RESPONSE_BREAK_OTHERS_A)
		{
			// The pin we are creating from already has a connection that needs to be broken. We want to "insert" the new node in between, so that the output of the new node is hooked up too
			UEdGraphPin* OldLinkedPin = FromPin->LinkedTo[0];
			check(OldLinkedPin);

			FromPin->BreakAllPinLinks();

			// Hook up the old linked pin to the first valid output pin on the new node
			for (UEdGraphPin* InnerPin : Pins)
			{
				if (Schema.CanCreateConnection(OldLinkedPin, InnerPin).Response != CONNECT_RESPONSE_MAKE)
				{
					continue;
				}

				Schema.TryCreateConnection(OldLinkedPin, InnerPin);
				break;
			}

			Schema.TryCreateConnection(FromPin, Pin);
			return;
		}
	}
}

void UVoxelGraphNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	// If the default value is manually set then treat it as if the value was reset to default and remove the orphaned pin
	if (Pin->bOrphanedPin &&
		Pin->DoesDefaultValueMatchAutogenerated())
	{
		PinConnectionListChanged(Pin);
	}

	GVoxelGraphTracker->NotifyEdGraphNodeChanged(*this);
}

void UVoxelGraphNode::PinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::PinConnectionListChanged(Pin);

	ON_SCOPE_EXIT
	{
		GVoxelGraphTracker->NotifyEdGraphNodeChanged(*this);
	};

	if (!Pin->bOrphanedPin ||
		Pin->LinkedTo.Num() > 0 ||
		!Pin->DoesDefaultValueMatchAutogenerated())
	{
		return;
	}

	// If we're not linked and this pin should no longer exist as part of the node, remove it

	RemovePin(Pin);
	RefreshNode();
}

void UVoxelGraphNode::NodeConnectionListChanged()
{
	Super::NodeConnectionListChanged();

	GVoxelGraphTracker->NotifyEdGraphNodeChanged(*this);
}

bool UVoxelGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA<UVoxelGraphSchema>();
}

void UVoxelGraphNode::PostDuplicate(const bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		CreateNewGuid();
	}
}

void UVoxelGraphNode::PostEditUndo()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditUndo();

	GVoxelGraphTracker->NotifyEdGraphNodeChanged(*this);
}

void UVoxelGraphNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshNode();

	GVoxelGraphTracker->NotifyEdGraphNodeChanged(*this);
}