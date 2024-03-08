// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_Parameter.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphTracker.h"

const FVoxelParameter* UVoxelGraphNode_Parameter::GetParameter() const
{
	const UVoxelGraph* Graph = GetTypedOuter<UVoxelGraph>();
	if (!ensure(Graph))
	{
		return nullptr;
	}

	const FVoxelParameter* Parameter = Graph->FindParameter(Guid);
	if (!Parameter)
	{
		return nullptr;
	}

	ConstCast(this)->CachedParameter = *Parameter;
	return Parameter;
}

FVoxelParameter UVoxelGraphNode_Parameter::GetParameterSafe() const
{
	if (const auto* Parameter = GetParameter())
	{
		return *Parameter;
	}

	return CachedParameter;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Parameter::AllocateDefaultPins()
{
	const UVoxelGraph* Graph = GetTypedOuter<UVoxelGraph>();
	if (ensure(Graph))
	{
		OnParameterChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnParameterChanged(*Graph).Add(FOnVoxelGraphChanged::Make(OnParameterChangedPtr, this, [=]
		{
			ReconstructNode();
		}));
	}

	const FVoxelParameter Parameter = GetParameterSafe();

	FVoxelPinType Type = Parameter.Type;
	if (bIsBuffer)
	{
		Type = Type.GetBufferType();
	}

	UEdGraphPin* Pin = CreatePin(
		EGPD_Output,
		Type.GetEdGraphPinType(),
		STATIC_FNAME("Value"));

	Pin->PinFriendlyName = FText::FromName(Parameter.Name);

	Super::AllocateDefaultPins();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Parameter::PrepareForCopying()
{
	Super::PrepareForCopying();

	// Updated CachedParameter
	(void)GetParameter();
}

void UVoxelGraphNode_Parameter::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelGraph* Graph = GetTypedOuter<UVoxelGraph>();
	if (!ensure(Graph))
	{
		return;
	}

	// No parameters in function libraries
	if (Graph->IsFunctionLibrary())
	{
		Guid = {};
		return;
	}

	if (Graph->FindParameter(Guid))
	{
		return;
	}

	for (const FGuid& ParameterGuid : Graph->GetParameters())
	{
		const FVoxelParameter& Parameter = Graph->FindParameterChecked(ParameterGuid);
		if (Parameter.Name != CachedParameter.Name ||
			Parameter.Type != CachedParameter.Type)
		{
			continue;
		}

		Guid = ParameterGuid;
		CachedParameter = Parameter;
		return;
	}

	// Add a new parameter
	// Regenerate guid to be safe
	Guid = FGuid::NewGuid();

	Graph->AddParameter(Guid, CachedParameter);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FText UVoxelGraphNode_Parameter::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	if (TitleType != ENodeTitleType::FullTitle)
	{
		return {};
	}

	return FText::FromString("Get " + GetParameterSafe().Name.ToString());
}

FLinearColor UVoxelGraphNode_Parameter::GetNodeTitleColor() const
{
	return GetSchema().GetPinTypeColor(GetParameterSafe().Type.GetEdGraphPinType());
}

FString UVoxelGraphNode_Parameter::GetSearchTerms() const
{
	return Guid.ToString();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphNode_Parameter::CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const
{
	if (Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	const FVoxelParameter* Parameter = GetParameter();
	if (!Parameter)
	{
		return false;
	}

	OutTypes.Add(Parameter->Type.GetInnerType());
	OutTypes.Add(Parameter->Type.GetBufferType());

	return true;
}

void UVoxelGraphNode_Parameter::PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType)
{
	Modify();

	ensure(bIsBuffer != NewType.IsBuffer());
	bIsBuffer = NewType.IsBuffer();

	ReconstructNode(false);
}