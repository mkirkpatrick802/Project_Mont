// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_Input.h"
#include "VoxelGraph.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphTracker.h"

UVoxelTerminalGraph* UVoxelGraphNode_Input::GetTerminalGraph() const
{
	UVoxelTerminalGraph* TerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(TerminalGraph))
	{
		return nullptr;
	}

	if (bIsGraphInput)
	{
		return &TerminalGraph->GetGraph().GetMainTerminalGraph();
	}

	return TerminalGraph;
}

const FVoxelGraphInput* UVoxelGraphNode_Input::GetInput() const
{
	const UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph();
	if (!ensure(TerminalGraph))
	{
		return nullptr;
	}

	const FVoxelGraphInput* Input = TerminalGraph->FindInput(Guid);
	if (!Input)
	{
		return nullptr;
	}

	ConstCast(this)->CachedInput = *Input;

	return Input;
}

FVoxelGraphInput UVoxelGraphNode_Input::GetInputSafe() const
{
	if (const FVoxelGraphInput* Input = GetInput())
	{
		return *Input;
	}

	return CachedInput;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Input::AllocateDefaultPins()
{
	const UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph();
	if (ensure(TerminalGraph))
	{
		OnInputChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnInputChanged(*TerminalGraph).Add(FOnVoxelGraphChanged::Make(OnInputChangedPtr, this, [=]
		{
			ReconstructNode();
		}));

		if (TerminalGraph->IsMainTerminalGraph())
		{
			// Main graph inputs always are graph inputs
			bIsGraphInput = true;
		}
		else
		{
			if (bIsGraphInput)
			{
				// Can't expose default outside the main graph
				bExposeDefaultPin = false;
			}
		}
	}

	const FVoxelGraphInput Input = GetInputSafe();

	if (bExposeDefaultPin)
	{
		UEdGraphPin* Pin = CreatePin(
			EGPD_Input,
			Input.Type.GetEdGraphPinType(),
			STATIC_FNAME("Default"));

		Pin->PinFriendlyName = FText::FromName(Input.Name);
	}

	{
		UEdGraphPin* Pin = CreatePin(
			EGPD_Output,
			CachedInput.Type.GetEdGraphPinType(),
			STATIC_FNAME("Value"));

		Pin->PinFriendlyName = FText::FromName(Input.Name);
	}

	Super::AllocateDefaultPins();
}

void UVoxelGraphNode_Input::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() != GET_OWN_MEMBER_NAME(bExposeDefaultPin))
	{
		return;
	}

	ReconstructNode();

	UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	// Force refresh inputs if bExposeDefaultPin changed
	GVoxelGraphTracker->NotifyInputChanged(*TerminalGraph);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Input::PrepareForCopying()
{
	Super::PrepareForCopying();

	// Updated CachedInput
	(void)GetInput();
}

void UVoxelGraphNode_Input::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelTerminalGraph* TerminalGraph = GetTerminalGraph();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	if (bIsGraphInput)
	{
		TerminalGraph = &TerminalGraph->GetGraph().GetMainTerminalGraph();
	}

	if (TerminalGraph->FindInput(Guid))
	{
		return;
	}

	for (const FGuid& InputGuid : TerminalGraph->GetInputs())
	{
		const FVoxelGraphInput& Input = TerminalGraph->FindInputChecked(InputGuid);
		if (Input.Name != CachedInput.Name ||
			Input.Type != CachedInput.Type)
		{
			continue;
		}

		Guid = InputGuid;
		CachedInput = Input;
		return;
	}

	// Try to find a matching input in the main graph
	if (!TerminalGraph->IsMainTerminalGraph())
	{
		ensure(!bIsGraphInput);

		const UVoxelTerminalGraph& MainTerminalGraph = TerminalGraph->GetGraph().GetMainTerminalGraph();

		for (const FGuid& InputGuid : MainTerminalGraph.GetInputs())
		{
			const FVoxelGraphInput& Input = MainTerminalGraph.FindInputChecked(InputGuid);
			if (Input.Name != CachedInput.Name ||
				Input.Type != CachedInput.Type)
			{
				continue;
			}

			Guid = InputGuid;
			CachedInput = Input;
			bIsGraphInput = true;
			return;
		}
	}

	// Add a new input
	// Regenerate guid to be safe
	Guid = FGuid::NewGuid();

	TerminalGraph->AddInput(Guid, CachedInput);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FText UVoxelGraphNode_Input::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return bIsGraphInput ? INVTEXT("GRAPH INPUT") : INVTEXT("INPUT");
}

FLinearColor UVoxelGraphNode_Input::GetNodeTitleColor() const
{
	return GetSchema().GetPinTypeColor(GetInputSafe().Type.GetEdGraphPinType());
}

FString UVoxelGraphNode_Input::GetSearchTerms() const
{
	return Guid.ToString();
}