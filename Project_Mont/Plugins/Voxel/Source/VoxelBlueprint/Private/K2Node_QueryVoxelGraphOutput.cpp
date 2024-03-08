// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "K2Node_QueryVoxelGraphOutput.h"

#include "VoxelGraph.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "BlueprintLibrary/VoxelGraphBlueprintLibrary.h"

#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"

const FName UK2Node_QueryVoxelGraphOutput::GraphPinName = "Graph";
const FName UK2Node_QueryVoxelGraphOutput::OutputNamePinName = "OutputName";

UK2Node_QueryVoxelGraphOutput::UK2Node_QueryVoxelGraphOutput()
{
	FunctionReference.SetExternalMember(
		GET_FUNCTION_NAME_CHECKED(UVoxelGraphBlueprintLibrary, K2_QueryVoxelGraphOutput),
		UVoxelGraphBlueprintLibrary::StaticClass());

	CachedGraphOutput.Type = FVoxelPinType::MakeWildcard();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UK2Node_QueryVoxelGraphOutput::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	if (UEdGraphPin* ParameterNamePin = FindPin(STATIC_FNAME("Name")))
	{
		ParameterNamePin->bHidden = true;
	}

	UEdGraphPin* ParameterPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, OutputNamePinName);
	ParameterPin->DefaultValue = CachedGraphOutput.GetValue();
}

void UK2Node_QueryVoxelGraphOutput::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	if (!Context->Pin ||
		!IsPinWildcard(*Context->Pin))
	{
		return;
	}

	if (CachedGraphOutput.Guid.IsValid())
	{
		const UEdGraphPin* OutputNamePin = FindPin(OutputNamePinName);
		if (!ensure(OutputNamePin) ||
			OutputNamePin->LinkedTo.Num() == 0)
		{
			return;
		}
	}

	AddConvertPinContextAction(Menu, Context, FVoxelPinTypeSet::AllUniforms());
}

void UK2Node_QueryVoxelGraphOutput::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin->PinName == OutputNamePinName)
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			return;
		}

		FGuid ParameterGuid;
		if (FGuid::ParseExact(Pin->DefaultValue, EGuidFormats::Digits, ParameterGuid))
		{
			if (ensure(CachedGraph))
			{
				if (const FVoxelGraphOutput* Output = CachedGraph->GetMainTerminalGraph().FindOutput(ParameterGuid))
				{
					SetGraphOutput(FVoxelGraphBlueprintOutput(ParameterGuid, *Output));
					return;
				}
			}

			FVoxelGraphBlueprintOutput InvalidGraphOutput = CachedGraphOutput;
			InvalidGraphOutput.bIsValid = false;
			SetGraphOutput(InvalidGraphOutput);
			return;
		}

		FVoxelGraphBlueprintOutput NewGraphOutput = CachedGraphOutput;
		NewGraphOutput.Guid = {};
		NewGraphOutput.Name = *Pin->DefaultValue;
		NewGraphOutput.bIsValid = true;
		SetGraphOutput(NewGraphOutput);
		return;
	}

	if (Pin->PinName == GraphPinName)
	{
		const FVoxelTransaction Transaction(this, "Set graph");

		if (Pin->DefaultObject != CachedGraph)
		{
			OnOutputsChangedPtr.Reset();

			if (const UVoxelGraph* Graph = Cast<UVoxelGraph>(Pin->DefaultObject))
			{
				OnOutputsChangedPtr = MakeSharedVoid();
				GVoxelGraphTracker->OnOutputChanged(Graph->GetMainTerminalGraph()).Add(FOnVoxelGraphChanged::Make(OnOutputsChangedPtr, this, [this]
				{
					FixupGraphOutput();
				}));
			}
		}

		CachedGraph = Cast<UVoxelGraph>(Pin->DefaultObject);

		const UEdGraphPin* OutputNamePin = FindPin(OutputNamePinName);
		if (!ensure(OutputNamePin) ||
			OutputNamePin->LinkedTo.Num() > 0)
		{
			return;
		}

		FixupGraphOutput();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UK2Node_QueryVoxelGraphOutput::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UEdGraphPin* GraphPin = FindPin(GraphPinName);
	if (!ensure(GraphPin))
	{
		return;
	}

	RemovePin(GraphPin);

	UEdGraphPin* OutputNamePin = FindPin(OutputNamePinName);
	if (!ensure(OutputNamePin))
	{
		return;
	}

	if (OutputNamePin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* NamePin = FindPin(STATIC_FNAME("Name"));
		if (!ensure(NamePin))
		{
			return;
		}

		CompilerContext.MovePinLinksToIntermediate(*OutputNamePin, *NamePin);
	}
	else if (
		CachedGraph &&
		CachedGraphOutput.Guid.IsValid() &&
		!CachedGraph->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		if (!CachedGraph->GetMainTerminalGraph().FindOutput(CachedGraphOutput.Guid))
		{
			CompilerContext.MessageLog.Error(*FString("Could not find a variable named \"" + CachedGraphOutput.Name.ToString() + "\" @@"), this);
			CachedGraphOutput.bIsValid = false;
			return;
		}
	}

	RemovePin(OutputNamePin);

	Super::ExpandNode(CompilerContext, SourceGraph);
}

void UK2Node_QueryVoxelGraphOutput::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	FEdGraphPinType OldPinType = Pin->PinType;

	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->PinName == OutputNamePinName)
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			FixupGraphOutput();
		}

		return;
	}

	if (!IsPinWildcard(*Pin))
	{
		return;
	}

	if (UEdGraphPin* OutputNamePin = FindPin(OutputNamePinName))
	{
		if (OutputNamePin->LinkedTo.Num() == 0 &&
			CachedGraphOutput.Guid.IsValid())
		{
			if (OldPinType != Pin->PinType)
			{
				Pin->PinType = OldPinType;
			}
			return;
		}
	}

	FEdGraphPinType TargetType = OldPinType;
	if (Pin->LinkedTo.Num() == 1)
	{
		UEdGraphPin* LinkedPin = Pin->LinkedTo[0];
		if (!ensure(LinkedPin))
		{
			return;
		}

		TargetType = LinkedPin->PinType;
	}

	const FVoxelPinType NewType = FVoxelPinType::MakeFromK2(TargetType);
	if (!NewType.IsValid())
	{
		if (!CachedGraphOutput.Guid.IsValid())
		{
			CachedGraphOutput.Type = {};
		}
		return;
	}

	if (TargetType == OldPinType)
	{
		if (CachedGraphOutput.Guid.IsValid() &&
			!ensure(CachedGraphOutput.Type == NewType))
		{
			return;
		}

		if (Pin->LinkedTo.Num() == 0 &&
			Pin->PinType != TargetType)
		{
			ReconstructNode();
		}
		return;
	}

	if (CachedGraphOutput.Type == NewType)
	{
		return;
	}

	if (!ensure(!NewType.IsWildcard()))
	{
		return;
	}

	FVoxelGraphBlueprintOutput NewGraphOutput = CachedGraphOutput;
	NewGraphOutput.bIsValid = true;
	NewGraphOutput.Type = NewType;

	if (Pin->LinkedTo.Num() > 0)
	{
		Pin->PinType = TargetType;
	}

	SetGraphOutput(NewGraphOutput);
}

void UK2Node_QueryVoxelGraphOutput::PostReconstructNode()
{
	Super::PostReconstructNode();

	if (UEdGraphPin* OutputNamePin = FindPin(OutputNamePinName))
	{
		OutputNamePin->DefaultValue = CachedGraphOutput.GetValue();
	}

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	for (UEdGraphPin* Pin : GetAllPins())
	{
		if (!IsPinWildcard(*Pin))
		{
			continue;
		}

		Pin->PinType = CachedGraphOutput.Type.GetEdGraphPinType_K2();
		if (Pin->DoesDefaultValueMatchAutogenerated())
		{
			Schema->ResetPinToAutogeneratedDefaultValue(Pin, false);
		}
	}
}

bool UK2Node_QueryVoxelGraphOutput::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (!IsPinWildcard(*MyPin))
	{
		return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
	}

	if (MyPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard ||
		CachedGraphOutput.Guid.IsValid())
	{
		return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
	}

	if (!CanAutoConvert(*MyPin, *OtherPin, OutReason))
	{
		return true;
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_QueryVoxelGraphOutput::PostLoad()
{
	Super::PostLoad();

	FixupGraphOutput();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UK2Node_QueryVoxelGraphOutput::OnPinTypeChange(UEdGraphPin& Pin, const FVoxelPinType& NewType)
{
	FVoxelGraphBlueprintOutput NewGraphOutput = CachedGraphOutput;
	NewGraphOutput.Type = NewType;
	NewGraphOutput.bIsValid = true;

	SetGraphOutput(NewGraphOutput);
}

bool UK2Node_QueryVoxelGraphOutput::IsPinWildcard(const UEdGraphPin& Pin) const
{
	return Pin.PinName == STATIC_FNAME("Value");
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UK2Node_QueryVoxelGraphOutput::FixupGraphOutput()
{
	UEdGraphPin* OutputNamePin = FindPin(OutputNamePinName);
	if (!ensure(OutputNamePin))
	{
		return;
	}

	if (!CachedGraph)
	{
		OnOutputsChangedPtr.Reset();

		const FVoxelTransaction Transaction(this, "Change Graph Output Pin Value");
		GetSchema()->TrySetDefaultValue(*OutputNamePin, CachedGraphOutput.Name.ToString());
		return;
	}

	// Wait for graph to fully load
	if (CachedGraph->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		return;
	}

	const UVoxelGraph* Graph = CachedGraph->GetGraph();
	if (!Graph)
	{
		OnOutputsChangedPtr.Reset();

		const FVoxelTransaction Transaction(this, "Change Graph Output Pin Value");
		GetSchema()->TrySetDefaultValue(*OutputNamePin, CachedGraphOutput.Name.ToString());
		return;
	}

	FGuid ExistingGraphOutputGuid;
	TOptional<FVoxelGraphOutput> ExistingGraphOutput;
	if (CachedGraphOutput.Guid.IsValid())
	{
		if (const FVoxelGraphOutput* GraphOutput = CachedGraph->GetMainTerminalGraph().FindOutput(CachedGraphOutput.Guid))
		{
			ExistingGraphOutputGuid = CachedGraphOutput.Guid;
			ExistingGraphOutput = *GraphOutput;
		}
	}
	else
	{
		if (const FVoxelGraphOutput* GraphOutput = CachedGraph->GetMainTerminalGraph().FindOutputByName(CachedGraphOutput.Name, ExistingGraphOutputGuid))
		{
			ExistingGraphOutput = *GraphOutput;
		}
	}

	if (ExistingGraphOutput)
	{
		if (CachedGraphOutput.Guid != ExistingGraphOutputGuid ||
			CachedGraphOutput.Name != ExistingGraphOutput->Name ||
			CachedGraphOutput.Type != ExistingGraphOutput->Type)
		{
			const FVoxelTransaction Transaction(this, "Change Graph Output Pin Value");
			GetSchema()->TrySetDefaultValue(*OutputNamePin, ExistingGraphOutputGuid.ToString());
		}
	}
	else
	{
		const FVoxelTransaction Transaction(this, "Change Graph Output Pin Value");
		GetSchema()->TrySetDefaultValue(*OutputNamePin, CachedGraphOutput.Guid.ToString());
	}

	if (!OnOutputsChangedPtr)
	{
		OnOutputsChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnOutputChanged(Graph->GetMainTerminalGraph()).Add(FOnVoxelGraphChanged::Make(OnOutputsChangedPtr, this, [this]
		{
			FixupGraphOutput();
		}));
	}
}

void UK2Node_QueryVoxelGraphOutput::SetGraphOutput(FVoxelGraphBlueprintOutput NewGraphOutput)
{
	const FVoxelTransaction Transaction(this, "Change Graph Output Pin Value");

	NewGraphOutput.Type = NewGraphOutput.Type.GetExposedType();

	const bool bSameType = NewGraphOutput.Type == CachedGraphOutput.Type;
	if (CachedGraphOutput.Guid != NewGraphOutput.Guid ||
		CachedGraphOutput.Name != NewGraphOutput.Name ||
		CachedGraphOutput.Type != NewGraphOutput.Type ||
		CachedGraphOutput.bIsValid != NewGraphOutput.bIsValid)
	{
		CachedGraphOutput = NewGraphOutput;
	}

	UEdGraphPin* NamePin = FindPin(STATIC_FNAME("Name"));
	if (!ensure(NamePin))
	{
		return;
	}

	NamePin->DefaultValue = CachedGraphOutput.Name.ToString();

	if (!bSameType)
	{
		FEdGraphPinType EdGraphType = NewGraphOutput.Type.GetEdGraphPinType_K2();

		for (UEdGraphPin* Pin : GetAllPins())
		{
			if (IsPinWildcard(*Pin) &&
				Pin->PinType != EdGraphType)
			{
				if (Pin->PinFriendlyName.IsEmpty())
				{
					Pin->PinFriendlyName = FText::FromName(Pin->PinName);
				}

				Pin->PinName = CreateUniquePinName(Pin->PinName);
			}
		}

		ReconstructNode();
	}

	// Let the graph know to refresh
	GetGraph()->NotifyGraphChanged();

	UBlueprint* Blueprint = GetBlueprint();
	if (!Blueprint->bBeingCompiled)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		Blueprint->BroadcastChanged();
	}
}