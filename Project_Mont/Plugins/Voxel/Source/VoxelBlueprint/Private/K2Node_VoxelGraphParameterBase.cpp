// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "K2Node_VoxelGraphParameterBase.h"
#include "VoxelGraph.h"
#include "VoxelGraphTracker.h"
#include "VoxelParameterView.h"
#include "VoxelGraphParametersView.h"
#include "Widgets/SVoxelPinTypeSelector.h"

#include "ToolMenu.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"

const FName UK2Node_VoxelGraphParameterBase::AssetPinName = "Asset";
const FName UK2Node_VoxelGraphParameterBase::ParameterPinName = "Parameter";

UK2Node_VoxelGraphParameterBase::UK2Node_VoxelGraphParameterBase()
{
	CachedParameter.Type = FVoxelPinType::MakeWildcard();
}

void UK2Node_VoxelGraphParameterBase::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	if (UEdGraphPin* ParameterNamePin = GetParameterNamePin())
	{
		ParameterNamePin->bHidden = true;
	}

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Interface, UVoxelGraph::StaticClass(), AssetPinName);

	UEdGraphPin* ParameterPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, ParameterPinName);
	ParameterPin->DefaultValue = CachedParameter.GetValue();
}

void UK2Node_VoxelGraphParameterBase::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);

	if (!Context->Pin ||
		!IsPinWildcard(*Context->Pin))
	{
		return;
	}

	if (CachedParameter.Guid.IsValid())
	{
		const UEdGraphPin* ParameterPin = FindPin(ParameterPinName);
		if (!ensure(ParameterPin) ||
			ParameterPin->LinkedTo.Num() == 0)
		{
			return;
		}
	}

	AddConvertPinContextAction(Menu, Context, FVoxelPinTypeSet::AllUniforms());
}

void UK2Node_VoxelGraphParameterBase::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin->PinName == ParameterPinName)
	{
		if (Pin->LinkedTo.Num() > 0)
		{
			return;
		}

		FGuid ParameterGuid;
		if (FGuid::ParseExact(Pin->DefaultValue, EGuidFormats::Digits, ParameterGuid))
		{
			if (ensure(CachedParameterOverridesOwner))
			{
				if (const TSharedPtr<FVoxelGraphParametersView> ParametersView = CachedParameterOverridesOwner->GetParametersView())
				{
					if (const FVoxelParameterView* ParameterView = ParametersView->FindByGuid(ParameterGuid))
					{
						SetParameter(FVoxelGraphBlueprintParameter(ParameterGuid, ParameterView->GetParameter()));
						return;
					}
				}
			}

			FVoxelGraphBlueprintParameter InvalidParameter = CachedParameter;
			InvalidParameter.bIsValid = false;
			SetParameter(InvalidParameter);
			return;
		}

		FVoxelGraphBlueprintParameter NewParameter = CachedParameter;
		NewParameter.Guid = {};
		NewParameter.Name = *Pin->DefaultValue;
		NewParameter.bIsValid = true;
		SetParameter(NewParameter);
		return;
	}

	if (Pin->PinName == AssetPinName)
	{
		const FVoxelTransaction Transaction(this, "Set asset reference");

		if (Pin->DefaultObject != CachedParameterOverridesOwner.GetObject())
		{
			OnParameterLayoutChangedPtr.Reset();

			if (const IVoxelParameterOverridesObjectOwner* ParameterOverridesOwner = Cast<IVoxelParameterOverridesObjectOwner>(Pin->DefaultObject))
			{
				OnParameterLayoutChangedPtr = MakeSharedVoid();
				ParameterOverridesOwner->OnParameterLayoutChanged.Add(MakeWeakPtrDelegate(OnParameterLayoutChangedPtr, MakeWeakObjectPtrLambda(this, [this]
				{
					FixupParameter();
				})));
			}
		}

		CachedParameterOverridesOwner = Pin->DefaultObject;

		const UEdGraphPin* ParameterPin = FindPin(ParameterPinName);
		if (!ensure(ParameterPin) ||
			ParameterPin->LinkedTo.Num() > 0)
		{
			return;
		}

		FixupParameter();
	}
}

void UK2Node_VoxelGraphParameterBase::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UEdGraphPin* AssetPin = FindPin(AssetPinName);
	if (!ensure(AssetPin))
	{
		return;
	}

	RemovePin(AssetPin);

	UEdGraphPin* ParameterPin = FindPin(ParameterPinName);
	if (!ensure(ParameterPin))
	{
		return;
	}

	if (ParameterPin->LinkedTo.Num() > 0)
	{
		UEdGraphPin* NamePin = GetParameterNamePin();
		if (!ensure(NamePin))
		{
			return;
		}

		CompilerContext.MovePinLinksToIntermediate(*ParameterPin, *NamePin);
	}
	else if (
		CachedParameterOverridesOwner &&
		CachedParameter.Guid.IsValid() &&
		!CachedParameterOverridesOwner.GetObject()->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		const TSharedPtr<FVoxelGraphParametersView> ParametersView = CachedParameterOverridesOwner->GetParametersView();
		if (!ParametersView ||
			!ParametersView->FindByGuid(CachedParameter.Guid))
		{
			CompilerContext.MessageLog.Error(*FString("Could not find a variable named \"" + CachedParameter.Name.ToString() + "\" @@"), this);
			CachedParameter.bIsValid = false;
			return;
		}
	}

	RemovePin(ParameterPin);

	Super::ExpandNode(CompilerContext, SourceGraph);
}

void UK2Node_VoxelGraphParameterBase::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	FEdGraphPinType OldPinType = Pin->PinType;

	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin->PinName == ParameterPinName)
	{
		if (Pin->LinkedTo.Num() == 0)
		{
			FixupParameter();
		}

		return;
	}

	if (!IsPinWildcard(*Pin))
	{
		return;
	}

	if (UEdGraphPin* ParameterPin = FindPin(ParameterPinName))
	{
		if (ParameterPin->LinkedTo.Num() == 0 &&
			CachedParameter.Guid.IsValid())
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
		if (!CachedParameter.Guid.IsValid())
		{
			CachedParameter.Type = {};
		}
		return;
	}

	if (TargetType == OldPinType)
	{
		if (CachedParameter.Guid.IsValid() &&
			!ensure(CachedParameter.Type == NewType))
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

	if (CachedParameter.Type == NewType)
	{
		return;
	}

	if (!ensure(!NewType.IsWildcard()))
	{
		return;
	}

	FVoxelGraphBlueprintParameter NewParameter = CachedParameter;
	NewParameter.bIsValid = true;
	NewParameter.Type = NewType;

	if (Pin->LinkedTo.Num() > 0)
	{
		Pin->PinType = TargetType;
	}

	SetParameter(NewParameter);
}

void UK2Node_VoxelGraphParameterBase::PostReconstructNode()
{
	Super::PostReconstructNode();

	if (UEdGraphPin* ParameterPin = FindPin(ParameterPinName))
	{
		ParameterPin->DefaultValue = CachedParameter.GetValue();
	}

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	for (UEdGraphPin* Pin : GetAllPins())
	{
		if (!IsPinWildcard(*Pin))
		{
			continue;
		}

		Pin->PinType = CachedParameter.Type.GetEdGraphPinType_K2();
		if (Pin->DoesDefaultValueMatchAutogenerated())
		{
			Schema->ResetPinToAutogeneratedDefaultValue(Pin, false);
		}
	}
}

bool UK2Node_VoxelGraphParameterBase::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (!IsPinWildcard(*MyPin))
	{
		return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
	}

	if (MyPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard ||
		CachedParameter.Guid.IsValid())
	{
		return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
	}

	if (!CanAutoConvert(*MyPin, *OtherPin, OutReason))
	{
		return true;
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_VoxelGraphParameterBase::PostLoad()
{
	Super::PostLoad();

	FixupParameter();
}

void UK2Node_VoxelGraphParameterBase::OnPinTypeChange(UEdGraphPin& Pin, const FVoxelPinType& NewType)
{
	FVoxelGraphBlueprintParameter NewParameter = CachedParameter;
	NewParameter.Type = NewType;
	NewParameter.bIsValid = true;

	SetParameter(NewParameter);
}

void UK2Node_VoxelGraphParameterBase::FixupParameter()
{
	UEdGraphPin* ParameterPin = FindPin(ParameterPinName);
	if (!ensure(ParameterPin))
	{
		return;
	}

	if (!CachedParameterOverridesOwner)
	{
		OnParameterLayoutChangedPtr.Reset();

		const FVoxelTransaction Transaction(this, "Change Parameter Pin Value");
		GetSchema()->TrySetDefaultValue(*ParameterPin, CachedParameter.Name.ToString());
		return;
	}

	// Wait for graph to fully load
	if (CachedParameterOverridesOwner.GetObject()->HasAnyFlags(RF_NeedLoad | RF_NeedPostLoad))
	{
		return;
	}

	const UVoxelGraph* Graph = CachedParameterOverridesOwner->GetGraph();
	if (!Graph)
	{
		OnParameterLayoutChangedPtr.Reset();

		const FVoxelTransaction Transaction(this, "Change Parameter Pin Value");
		GetSchema()->TrySetDefaultValue(*ParameterPin, CachedParameter.Name.ToString());
		return;
	}
	const TSharedRef<FVoxelGraphParametersView> ParametersView = CachedParameterOverridesOwner->GetParametersView_ValidGraph();

	FGuid ExistingParameterGuid;
	TOptional<FVoxelParameter> ExistingParameter;
	if (CachedParameter.Guid.IsValid())
	{
		if (const FVoxelParameterView* ParameterView = ParametersView->FindByGuid(CachedParameter.Guid))
		{
			ExistingParameterGuid = ParameterView->GetGuid();
			ExistingParameter = ParameterView->GetParameter();
		}
	}
	else
	{
		if (const FVoxelParameterView* ParameterView = ParametersView->FindByName(CachedParameter.Name))
		{
			ExistingParameterGuid = ParameterView->GetGuid();
			ExistingParameter = ParameterView->GetParameter();
		}
	}

	if (ExistingParameter)
	{
		if (CachedParameter.Guid != ExistingParameterGuid ||
			CachedParameter.Name != ExistingParameter->Name ||
			CachedParameter.Type != ExistingParameter->Type)
		{
			const FVoxelTransaction Transaction(this, "Change Parameter Pin Value");
			GetSchema()->TrySetDefaultValue(*ParameterPin, ExistingParameterGuid.ToString());
		}
	}
	else
	{
		const FVoxelTransaction Transaction(this, "Change Parameter Pin Value");
		GetSchema()->TrySetDefaultValue(*ParameterPin, CachedParameter.Guid.ToString());
	}

	if (!OnParameterLayoutChangedPtr)
	{
		OnParameterLayoutChangedPtr = MakeSharedVoid();
		CachedParameterOverridesOwner->OnParameterLayoutChanged.Add(MakeWeakPtrDelegate(OnParameterLayoutChangedPtr, MakeWeakObjectPtrLambda(this, [this]
		{
			FixupParameter();
		})));
	}
}

void UK2Node_VoxelGraphParameterBase::SetParameter(FVoxelGraphBlueprintParameter NewParameter)
{
	const FVoxelTransaction Transaction(this, "Change Parameter Pin Value");

	NewParameter.Type = NewParameter.Type.GetExposedType();

	const bool bSameType = NewParameter.Type == CachedParameter.Type;
	if (CachedParameter.Guid != NewParameter.Guid ||
		CachedParameter.Name != NewParameter.Name ||
		CachedParameter.Type != NewParameter.Type ||
		CachedParameter.bIsValid != NewParameter.bIsValid)
	{
		CachedParameter = NewParameter;
	}

	UEdGraphPin* NamePin = GetParameterNamePin();
	if (!ensure(NamePin))
	{
		return;
	}

	NamePin->DefaultValue = CachedParameter.Name.ToString();

	if (!bSameType)
	{
		FEdGraphPinType EdGraphType = NewParameter.Type.GetEdGraphPinType_K2();

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