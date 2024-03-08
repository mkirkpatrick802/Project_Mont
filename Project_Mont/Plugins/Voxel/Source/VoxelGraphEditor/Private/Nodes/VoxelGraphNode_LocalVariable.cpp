// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_LocalVariable.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphTracker.h"

const FVoxelGraphLocalVariable* UVoxelGraphNode_LocalVariable::GetLocalVariable() const
{
	const UVoxelTerminalGraph* TerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(TerminalGraph))
	{
		return nullptr;
	}

	const FVoxelGraphLocalVariable* LocalVariable = TerminalGraph->FindLocalVariable(Guid);
	if (!LocalVariable)
	{
		return nullptr;
	}

	ConstCast(this)->CachedLocalVariable = *LocalVariable;

	return LocalVariable;
}

FVoxelGraphLocalVariable UVoxelGraphNode_LocalVariable::GetLocalVariableSafe() const
{
	if (const FVoxelGraphLocalVariable* LocalVariable = GetLocalVariable())
	{
		return *LocalVariable;
	}

	return CachedLocalVariable;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_LocalVariable::AllocateDefaultPins()
{
	const UVoxelTerminalGraph* TerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (ensure(TerminalGraph))
	{
		OnLocalVariableChangedPtr = MakeSharedVoid();
		GVoxelGraphTracker->OnLocalVariableChanged(*TerminalGraph).Add(FOnVoxelGraphChanged::Make(OnLocalVariableChangedPtr, this, [=]
		{
			ReconstructNode();
		}));
	}

	Super::AllocateDefaultPins();
}

FLinearColor UVoxelGraphNode_LocalVariable::GetNodeTitleColor() const
{
	return GetSchema().GetPinTypeColor(GetLocalVariableSafe().Type.GetEdGraphPinType());
}

FString UVoxelGraphNode_LocalVariable::GetSearchTerms() const
{
	return Guid.ToString();
}

void UVoxelGraphNode_LocalVariable::PrepareForCopying()
{
	Super::PrepareForCopying();

	// Updated CachedLocalVariable
	(void)GetLocalVariable();
}

void UVoxelGraphNode_LocalVariable::PostPasteNode()
{
	Super::PostPasteNode();

	UVoxelTerminalGraph* TerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	if (TerminalGraph->FindLocalVariable(Guid))
	{
		return;
	}

	for (const FGuid& LocalVariableGuid : TerminalGraph->GetLocalVariables())
	{
		const FVoxelGraphLocalVariable& LocalVariable = TerminalGraph->FindLocalVariableChecked(LocalVariableGuid);
		if (LocalVariable.Name != CachedLocalVariable.Name ||
			LocalVariable.Type != CachedLocalVariable.Type)
		{
			continue;
		}

		Guid = LocalVariableGuid;
		CachedLocalVariable = LocalVariable;
		return;
	}

	// Add a new local variable
	// Regenerate guid to be safe
	Guid = FGuid::NewGuid();

	TerminalGraph->AddLocalVariable(Guid, CachedLocalVariable);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_LocalVariableDeclaration::AllocateDefaultPins()
{
	const FVoxelGraphLocalVariable LocalVariable = GetLocalVariableSafe();

	{
		UEdGraphPin* Pin = CreatePin(EGPD_Input, LocalVariable.Type.GetEdGraphPinType(), FName("InputPin"));
		Pin->bAllowFriendlyName = true;
		Pin->PinFriendlyName = FText::FromName(LocalVariable.Name);
	}

	{
		UEdGraphPin* Pin = CreatePin(EGPD_Output, LocalVariable.Type.GetEdGraphPinType(), FName("OutputPin"));
		Pin->bAllowFriendlyName = true;
		Pin->PinFriendlyName = INVTEXT(" ");
	}

	Super::AllocateDefaultPins();
}

FText UVoxelGraphNode_LocalVariableDeclaration::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	if (TitleType != ENodeTitleType::FullTitle)
	{
		return INVTEXT("LOCAL");
	}

	return FText::FromString("Set " + GetLocalVariableSafe().Name.ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_LocalVariableUsage::AllocateDefaultPins()
{
	const FVoxelGraphLocalVariable LocalVariable = GetLocalVariableSafe();

	UEdGraphPin* Pin = CreatePin(EGPD_Output, LocalVariable.Type.GetEdGraphPinType(), FName("OutputPin"));
	Pin->PinFriendlyName = FText::FromName(LocalVariable.Name);

	Super::AllocateDefaultPins();
}

FText UVoxelGraphNode_LocalVariableUsage::GetNodeTitle(const ENodeTitleType::Type TitleType) const
{
	if (TitleType != ENodeTitleType::FullTitle)
	{
		return {};
	}

	return FText::FromString("Get " + GetLocalVariableSafe().Name.ToString());
}

void UVoxelGraphNode_LocalVariableUsage::JumpToDefinition() const
{
	UVoxelGraphNode_LocalVariableDeclaration* DeclarationNode = FindDeclaration();
	if (!DeclarationNode)
	{
		return;
	}

	const TSharedPtr<SGraphEditor> ActiveGraphEditor = GetToolkit()->GetActiveGraphEditor();
	ActiveGraphEditor->ClearSelectionSet();
	ActiveGraphEditor->SetNodeSelection(DeclarationNode, true);
	ActiveGraphEditor->ZoomToFit(true);
}

UVoxelGraphNode_LocalVariableDeclaration* UVoxelGraphNode_LocalVariableUsage::FindDeclaration() const
{
	TArray<UVoxelGraphNode_LocalVariableDeclaration*> DeclarationNodes;
	GetGraph()->GetNodesOfClass<UVoxelGraphNode_LocalVariableDeclaration>(DeclarationNodes);

	for (UVoxelGraphNode_LocalVariableDeclaration* Node : DeclarationNodes)
	{
		if (Node->Guid == Guid)
		{
			return Node;
		}
	}

	return nullptr;
}