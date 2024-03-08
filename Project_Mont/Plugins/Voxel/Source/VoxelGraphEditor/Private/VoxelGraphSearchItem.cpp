// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphSearchItem.h"
#include "VoxelGraph.h"
#include "VoxelGraphVisuals.h"
#include "VoxelGraphToolkit.h"
#include "Widgets/SVoxelGraphSearch.h"
#include "Nodes/VoxelGraphNode_Input.h"
#include "Nodes/VoxelGraphNode_Output.h"
#include "Nodes/VoxelGraphNode_Parameter.h"
#include "Nodes/VoxelGraphNode_LocalVariable.h"

void FVoxelGraphSearchItem::Initialize(const TSharedRef<const FContext>& NewContext)
{
	ensure(!Context);
	Context = NewContext;
	Build();
}

void FVoxelGraphSearchItem::OnClick()
{
	const TSharedPtr<FVoxelGraphSearchItem> Parent = WeakParent.Pin();
	if (!Parent)
	{
		return;
	}

	Parent->OnClick();
}

bool FVoxelGraphSearchItem::HasToken(const FString& Token) const
{
	return Context->Tokens.Contains(Token);
}

bool FVoxelGraphSearchItem::MatchesString(const FString& String) const
{
	FString LocalString = String;
	if (Context->Settings.bRemoveSpaces)
	{
		LocalString = LocalString.Replace(TEXT(" "), TEXT(""));
	}

	for (const FString& Token : Context->Tokens)
	{
		if (!LocalString.Contains(Token))
		{
			return false;
		}
	}
	return true;
}


TSharedPtr<FVoxelGraphToolkit> FVoxelGraphSearchItem::OpenToolkit(const UVoxelGraph& Graph)
{
	FVoxelToolkit* Toolkit = FVoxelToolkit::OpenToolkit(Graph, FVoxelGraphToolkit::StaticStruct());
	if (!ensure(Toolkit))
	{
		return nullptr;
	}

	return StaticCastSharedRef<FVoxelGraphToolkit>(Toolkit->AsShared());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphSearchItem_Text::GetName() const
{
	if (Prefix.IsEmpty())
	{
		return Text;
	}

	return Prefix + ": "+ Text;
}

bool FVoxelGraphSearchItem_Text::Matches() const
{
	return MatchesString(Text);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphSearchItem_Graph::GetName() const
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return {};
	}
	return Graph->GetPackage()->GetName();
}

const FSlateBrush* FVoxelGraphSearchItem_Graph::GetIcon(FSlateColor& OutColor) const
{
	return nullptr;
}

void FVoxelGraphSearchItem_Graph::Build()
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	for (const FGuid& Guid : Graph->GetParameters())
	{
		AddChild<FVoxelGraphSearchItem_Parameter>(Guid, Graph->FindParameterChecked(Guid));
	}

	for (const FGuid& Guid : Graph->GetTerminalGraphs())
	{
		AddChild<FVoxelGraphSearchItem_TerminalGraph>(Graph->FindTerminalGraphChecked(Guid));
	}
}

void FVoxelGraphSearchItem_Graph::OnClick()
{
	const UVoxelGraph* Graph = WeakGraph.Get();
	if (!ensure(Graph))
	{
		return;
	}

	OpenToolkit(*Graph);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphSearchItem_TerminalGraph::GetName() const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return {};
	}
	return TerminalGraph->GetDisplayName();
}

const FSlateBrush* FVoxelGraphSearchItem_TerminalGraph::GetIcon(FSlateColor& OutColor) const
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return nullptr;
	}

	if (TerminalGraph->HasExecNodes())
	{
		return FVoxelEditorStyle::GetBrush("VoxelGraph.Execute");
	}
	else
	{
		return FAppStyle::GetBrush("GraphEditor.Function_16x");
	}
}

void FVoxelGraphSearchItem_TerminalGraph::Build()
{
	const UVoxelTerminalGraph* TerminalGraph = WeakTerminalGraph.Get();
	if (!ensure(TerminalGraph))
	{
		return;
	}

	for (const FGuid& Guid : TerminalGraph->GetInputs())
	{
		AddChild<FVoxelGraphSearchItem_Input>(Guid, TerminalGraph->FindInputChecked(Guid));
	}
	for (const FGuid& Guid : TerminalGraph->GetOutputs())
	{
		AddChild<FVoxelGraphSearchItem_Output>(Guid, TerminalGraph->FindOutputChecked(Guid));
	}
	for (const FGuid& Guid : TerminalGraph->GetLocalVariables())
	{
		AddChild<FVoxelGraphSearchItem_LocalVariable>(Guid, TerminalGraph->FindLocalVariableChecked(Guid));
	}

	for (UEdGraphNode* Node : TerminalGraph->GetEdGraph().Nodes)
	{
		AddChild<FVoxelGraphSearchItem_Node>(Node);
	}
}

void FVoxelGraphSearchItem_TerminalGraph::OnClick()
{
	FVoxelUtilities::FocusObject(WeakTerminalGraph.Get());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphSearchItem_Node::GetName() const
{
	const UEdGraphNode* Node = WeakNode.Get();
	if (!ensure(Node))
	{
		return {};
	}

	return Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
}

FString FVoxelGraphSearchItem_Node::GetComment() const
{
	const UEdGraphNode* Node = WeakNode.Get();
	if (!ensure(Node))
	{
		return {};
	}

	return Node->NodeComment;
}

const FSlateBrush* FVoxelGraphSearchItem_Node::GetIcon(FSlateColor& OutColor) const
{
	const UEdGraphNode* Node = WeakNode.Get();
	if (!ensure(Node))
	{
		return nullptr;
	}

	FLinearColor Color = FLinearColor::White;
	const FSlateIcon Icon = Node->GetIconAndTint(Color);
	OutColor = Color;

	FLinearColor Dummy;
	if (Icon == GetDefault<UEdGraphNode>()->GetIconAndTint(Dummy))
	{
		return nullptr;
	}

	return Icon.GetIcon();
}

void FVoxelGraphSearchItem_Node::Build()
{
	const UEdGraphNode* Node = WeakNode.Get();
	if (!ensure(Node))
	{
		return;
	}

	if (const UVoxelGraphNode_Parameter* ParameterNode = Cast<UVoxelGraphNode_Parameter>(Node))
	{
		AddChild<FVoxelGraphSearchItem_Parameter>(ParameterNode->Guid, ParameterNode->GetParameterSafe());
		return;
	}
	if (const UVoxelGraphNode_Input* InputNode = Cast<UVoxelGraphNode_Input>(Node))
	{
		AddChild<FVoxelGraphSearchItem_Input>(InputNode->Guid, InputNode->GetInputSafe());
		return;
	}
	if (const UVoxelGraphNode_Output* OutputNode = Cast<UVoxelGraphNode_Output>(Node))
	{
		AddChild<FVoxelGraphSearchItem_Output>(OutputNode->Guid, OutputNode->GetOutputSafe());
		return;
	}
	if (const UVoxelGraphNode_LocalVariable* LocalVariableNode = Cast<UVoxelGraphNode_LocalVariable>(Node))
	{
		AddChild<FVoxelGraphSearchItem_LocalVariable>(LocalVariableNode->Guid, LocalVariableNode->GetLocalVariableSafe());
		return;
	}

	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::NodeTitle, "Title", Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Description, "Description", Node->GetTooltipText().ToString());

	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (!ensure(Pin))
		{
			continue;
		}

		AddChild<FVoxelGraphSearchItem_Pin>(Pin);
	}
}

void FVoxelGraphSearchItem_Node::OnClick()
{
	FVoxelUtilities::FocusObject(WeakNode.Get());
}

bool FVoxelGraphSearchItem_Node::Matches() const
{
	const UEdGraphNode* Node = WeakNode.Get();
	if (!ensure(Node))
	{
		return false;
	}

	return HasToken(Node->GetName());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphSearchItem_Pin::GetName() const
{
	const UEdGraphPin* Pin = WeakPin.Get();
	if (!ensure(Pin))
	{
		return {};
	}

	return Pin->GetSchema()->GetPinDisplayName(Pin).ToString();
}

const FSlateBrush* FVoxelGraphSearchItem_Pin::GetIcon(FSlateColor& OutColor) const
{
	const UEdGraphPin* Pin = WeakPin.Get();
	if (!ensure(Pin))
	{
		return {};
	}

	OutColor = FVoxelGraphVisuals::GetPinColor(Pin->PinType);
	return FAppStyle::GetBrush("GraphEditor.PinIcon");
}

void FVoxelGraphSearchItem_Pin::Build()
{
	const UEdGraphPin* Pin = WeakPin.Get();
	if (!ensure(Pin))
	{
		return;
	}

	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Type, "Type", FVoxelPinType(Pin->PinType).ToString());
	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Description, "Description", Pin->PinToolTip);

	if (FVoxelPinType(Pin->PinType).HasPinDefaultValue())
	{
		AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::DefaultValue, "Default Value", FVoxelPinValue::MakeFromPinDefaultValue(*Pin).ExportToString());
	}
}

bool FVoxelGraphSearchItem_Pin::Matches() const
{
	const UEdGraphPin* Pin = WeakPin.Get();
	if (!ensure(Pin))
	{
		return false;
	}

	return
		MatchesString(Pin->PinName.ToString()) ||
		HasToken(Pin->PinId.ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphSearchItem_Parameter::GetName() const
{
	return Parameter.Name.ToString();
}

const FSlateBrush* FVoxelGraphSearchItem_Parameter::GetIcon(FSlateColor& OutColor) const
{
	OutColor = FVoxelGraphVisuals::GetPinColor(Parameter.Type);
	return FVoxelGraphVisuals::GetPinIcon(Parameter.Type).GetOptionalIcon();
}

void FVoxelGraphSearchItem_Parameter::Build()
{
	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Type, "Type", Parameter.Type.ToString());
	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Description, "Description", Parameter.Description);
}

bool FVoxelGraphSearchItem_Parameter::Matches() const
{
	return HasToken(Guid.ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelGraphSearchItem_Property::GetName() const
{
	return Property.Name.ToString();
}

const FSlateBrush* FVoxelGraphSearchItem_Property::GetIcon(FSlateColor& OutColor) const
{
	OutColor = FVoxelGraphVisuals::GetPinColor(Property.Type);
	return FVoxelGraphVisuals::GetPinIcon(Property.Type).GetOptionalIcon();
}

void FVoxelGraphSearchItem_Property::Build()
{
	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Type, "Type", Property.Type.ToString());
	AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::Description, "Description", Property.Description);
}

bool FVoxelGraphSearchItem_Property::Matches() const
{
	return HasToken(Guid.ToString());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelGraphSearchItem_Input::Build()
{
	FVoxelGraphSearchItem_Property::Build();

	const FString DefaultValue = Input.DefaultValue.ExportToString();
	if (!DefaultValue.IsEmpty())
	{
		AddChild<FVoxelGraphSearchItem_Text>(EVoxelGraphSearchResultTag::DefaultValue, "Default Value", DefaultValue);
	}
}