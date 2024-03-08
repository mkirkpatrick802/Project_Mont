// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "K2Node_SetVoxelGraphParameter.h"
#include "K2Node_GetVoxelGraphParameter.h"
#include "BlueprintLibrary/VoxelParameterBlueprintLibrary.h"

#include "KismetCompiler.h"
#include "K2Node_MakeArray.h"
#include "K2Node_GetArrayItem.h"

UK2Node_SetVoxelGraphParameter::UK2Node_SetVoxelGraphParameter()
{
	FunctionReference.SetExternalMember(
		GET_FUNCTION_NAME_CHECKED(UVoxelParameterBlueprintLibrary, K2_SetVoxelParameter),
		UVoxelParameterBlueprintLibrary::StaticClass());
}

void UK2Node_SetVoxelGraphParameter::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	if (UEdGraphPin* OutValuePin = FindPin(STATIC_FNAME("OutValue")))
	{
		OutValuePin->PinFriendlyName = INVTEXT("Value");
	}

	{
		const int32 ValuePinIndex = Pins.IndexOfByPredicate([](const UEdGraphPin* Pin)
		{
			return Pin->PinName == STATIC_FNAME("Value");
		});
		const int32 AssetPinIndex = Pins.IndexOfByPredicate([](const UEdGraphPin* Pin)
		{
			return Pin->PinName == AssetPinName;
		});

		if (!ensure(ValuePinIndex != -1) ||
			!ensure(AssetPinIndex != -1))
		{
			return;
		}

		Pins.Swap(ValuePinIndex, AssetPinIndex);
	}

	{
		const int32 ValuePinIndex = Pins.IndexOfByPredicate([](const UEdGraphPin* Pin)
		{
			return Pin->PinName == STATIC_FNAME("Value");
		});
		const int32 ParameterPinIndex = Pins.IndexOfByPredicate([](const UEdGraphPin* Pin)
		{
			return Pin->PinName == ParameterPinName;
		});

		if (!ensure(ValuePinIndex != -1) ||
			!ensure(ParameterPinIndex != -1))
		{
			return;
		}

		Pins.Swap(ValuePinIndex, ParameterPinIndex);
	}
}

void UK2Node_SetVoxelGraphParameter::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* NamePin = FindPin(STATIC_FNAME("Name"));
	UEdGraphPin* OutValuePin = FindPin(STATIC_FNAME("OutValue"));
	UEdGraphPin* OwnerPin = FindPin(STATIC_FNAME("Owner"));

	if (!ensure(NamePin) ||
		!ensure(OutValuePin) ||
		!ensure(OwnerPin) ||
		OutValuePin->LinkedTo.Num() == 0)
	{
		return;
	}

	UK2Node_GetVoxelGraphParameter* GetterNode = CompilerContext.SpawnIntermediateNode<UK2Node_GetVoxelGraphParameter>(this, SourceGraph);
	GetterNode->AllocateDefaultPins();

	UEdGraphPin* FunctionNamePin = GetterNode->FindPin(STATIC_FNAME("Name"));
	UEdGraphPin* FunctionValuePin = GetterNode->FindPin(STATIC_FNAME("Value"));
	UEdGraphPin* FunctionOwnerPin = GetterNode->FindPin(STATIC_FNAME("Owner"));

	if (!ensure(FunctionNamePin) ||
		!ensure(FunctionValuePin) ||
		!ensure(FunctionOwnerPin))
	{
		return;
	}

	GetterNode->CachedParameter = CachedParameter;
	FunctionNamePin->DefaultValue = NamePin->DefaultValue;

	GetterNode->PostReconstructNode();

	CompilerContext.MovePinLinksToIntermediate(*GetThenPin(), *GetterNode->GetThenPin());
	CompilerContext.CopyPinLinksToIntermediate(*OwnerPin, *FunctionOwnerPin);
	CompilerContext.CopyPinLinksToIntermediate(*NamePin, *FunctionNamePin);
	CompilerContext.MovePinLinksToIntermediate(*OutValuePin, *FunctionValuePin);

	GetThenPin()->MakeLinkTo(GetterNode->GetExecPin());

	GetterNode->PostReconstructNode();

	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(GetterNode, this);
}

bool UK2Node_SetVoxelGraphParameter::IsPinWildcard(const UEdGraphPin& Pin) const
{
	return
		Pin.PinName == STATIC_FNAME("Value") ||
		Pin.PinName == STATIC_FNAME("OutValue");
}

UEdGraphPin* UK2Node_SetVoxelGraphParameter::GetParameterNamePin() const
{
	return FindPin(STATIC_FNAME("Name"));
}