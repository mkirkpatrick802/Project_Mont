// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_Output.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphTracker.h"

const FVoxelGraphOutput* UVoxelGraphNode_Output::GetOutput() const
{
	const UVoxelTerminalGraph* TerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(TerminalGraph))
	{
		return nullptr;
	}

	const FVoxelGraphOutput* Output = TerminalGraph->FindOutput(Guid);
	if (!Output)
	{
		return nullptr;
	}

	ConstCast(this)->CachedOutput = *Output;

	return Output;
}

FVoxelGraphOutput UVoxelGraphNode_Output::GetOutputSafe() const
{
	if (const FVoxelGraphOutput* Output = GetOutput())
	{
		return *Output;
	}

	return CachedOutput;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Output::AllocateDefaultPins()
{
	const UVoxelTerminalGraph* TerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (ensure(TerminalGraph))
	{
		OnOutputChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnOutputChanged(*TerminalGraph).Add(FOnVoxelGraphChanged::Make(OnOutputChangedPtr, this, [=]
		{
			ReconstructNode();
		}));
	}

	const FVoxelGraphOutput Output = GetOutputSafe();

	UEdGraphPin* Pin = CreatePin(
		EGPD_Input,
		Output.Type.GetEdGraphPinType(),
		STATIC_FNAME("Value"));

	Pin->PinFriendlyName = FText::FromName(Output.Name);

	Super::AllocateDefaultPins();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Output::PrepareForCopying()
{
	Super::PrepareForCopying();

	// Updated CachedOutput
	(void)GetOutput();
}

void UVoxelGraphNode_Output::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelTerminalGraph* TerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	if (TerminalGraph->FindOutput(Guid))
	{
		return;
	}

	for (const FGuid& OutputGuid : TerminalGraph->GetOutputs())
	{
		const FVoxelGraphOutput& Output = TerminalGraph->FindOutputChecked(OutputGuid);
		if (Output.Name != CachedOutput.Name ||
			Output.Type != CachedOutput.Type)
		{
			continue;
		}

		Guid = OutputGuid;
		CachedOutput = Output;
		return;
	}

	// Add a new output
	// Regenerate guid to be safe
	Guid = FGuid::NewGuid();

	TerminalGraph->AddOutput(Guid, CachedOutput);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FText UVoxelGraphNode_Output::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return INVTEXT("OUTPUT");
}

FLinearColor UVoxelGraphNode_Output::GetNodeTitleColor() const
{
	return GetSchema().GetPinTypeColor(GetOutputSafe().Type.GetEdGraphPinType());
}

FString UVoxelGraphNode_Output::GetSearchTerms() const
{
	return Guid.ToString();
}