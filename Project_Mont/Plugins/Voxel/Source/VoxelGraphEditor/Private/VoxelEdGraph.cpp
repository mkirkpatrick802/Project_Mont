// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelEdGraph.h"
#include "VoxelGraphSchema.h"
#include "VoxelGraphTracker.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphMigration.h"
#include "Nodes/VoxelLerpNodes.h"
#include "Nodes/VoxelNoiseNodes.h"
#include "Nodes/VoxelNode_UFunction.h"
#include "Nodes/VoxelGraphNode_Input.h"
#include "Nodes/VoxelGraphNode_Output.h"
#include "Nodes/VoxelGraphNode_Struct.h"
#include "Nodes/VoxelGraphNode_CallGraph.h"
#include "Nodes/VoxelGraphNode_CallFunction.h"
#include "Nodes/VoxelGraphNode_LocalVariable.h"
#include "GraphEditAction.h"

// Needed to load old graphs
constexpr FVoxelGuid GVoxelEdGraphCustomVersionGUID = MAKE_VOXEL_GUID("217DD26F36B54889936719C4ED363B8A");
FCustomVersionRegistration GRegisterVoxelEdGraphCustomVersionGUID(GVoxelEdGraphCustomVersionGUID, 1, TEXT("VoxelEdGraphVer"));

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelEdGraph::SetToolkit(const TSharedRef<FVoxelGraphToolkit>& Toolkit)
{
	ensure(!WeakToolkit.IsValid() || WeakToolkit == Toolkit);
	WeakToolkit = Toolkit;
}

TSharedPtr<FVoxelGraphToolkit> UVoxelEdGraph::GetGraphToolkit() const
{
	return WeakToolkit.Pin();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelEdGraph::MigrateIfNeeded()
{
	VOXEL_FUNCTION_COUNTER();

	if (Version != FVersion::LatestVersion)
	{
		MigrateAndReconstructAll();
	}

	ensure(Version == FVersion::LatestVersion);
}

void UVoxelEdGraph::MigrateAndReconstructAll()
{
	VOXEL_FUNCTION_COUNTER();

	// Disable SetDirty
	TGuardValue<bool> SetDirtyGuard(GIsEditorLoadingPackage, true);

	UVoxelTerminalGraph* OuterTerminalGraph = GetTypedOuter<UVoxelTerminalGraph>();
	if (!ensure(OuterTerminalGraph))
	{
		return;
	}

	ON_SCOPE_EXIT
	{
		// Migrate struct nodes
		for (UEdGraphNode* Node : Nodes)
		{
			UVoxelGraphNode_Struct* StructNode = Cast<UVoxelGraphNode_Struct>(Node);
			if (!StructNode ||
				StructNode->Struct.IsValid())
			{
				continue;
			}

			const FName CachedName(StructNode->CachedName);

			UFunction* NewFunction = GVoxelGraphMigration->FindNewFunction(CachedName);
			if (!NewFunction)
			{
				continue;
			}

			StructNode->Struct = FVoxelNode_UFunction::StaticStruct();
			StructNode->Struct.Get<FVoxelNode_UFunction>().SetFunction_EditorOnly(NewFunction);
			StructNode->ReconstructNode();

			GVoxelGraphMigration->OnNodeMigrated(CachedName, StructNode);
		}

		// Migrate function nodes
		for (UEdGraphNode* Node : Nodes)
		{
			UVoxelGraphNode_Struct* StructNode = Cast<UVoxelGraphNode_Struct>(Node);
			if (!StructNode ||
				!StructNode->Struct.IsA<FVoxelNode_UFunction>())
			{
				continue;
			}

			const FVoxelNode_UFunction& FunctionNode = StructNode->Struct->AsChecked<FVoxelNode_UFunction>();
			if (FunctionNode.GetFunction())
			{
				continue;
			}

			const FName CachedName = FunctionNode.GetCachedName();

			UScriptStruct* NewNode = GVoxelGraphMigration->FindNewNode(CachedName);
			if (!NewNode)
			{
				continue;
			}

			StructNode->Struct = NewNode;
			StructNode->ReconstructNode();

			GVoxelGraphMigration->OnNodeMigrated(CachedName, StructNode);
		}

		for (UEdGraphNode* Node : Nodes)
		{
			Node->ReconstructNode();
		}

		Version = FVersion::LatestVersion;
	};

	if (Version == FVersion::LatestVersion)
	{
		return;
	}

	if (Version < FVersion::SplitInputSetterAndRemoveLocalVariablesDefault)
	{
		TArray<UDEPRECATED_VoxelGraphMacroParameterNode*> ParameterNodes;
		GetNodesOfClass<UDEPRECATED_VoxelGraphMacroParameterNode>(ParameterNodes);

		for (UDEPRECATED_VoxelGraphMacroParameterNode* ParameterNode : ParameterNodes)
		{
			if (ParameterNode->Type == EVoxelGraphParameterType_DEPRECATED::Input)
			{
				FGraphNodeCreator<UVoxelGraphNode_Input> NodeCreator(*this);
				UVoxelGraphNode_Input* Node = NodeCreator.CreateNode(false);

				if (const FVoxelGraphInput* Input = OuterTerminalGraph->FindInput(ParameterNode->Guid))
				{
					Node->bExposeDefaultPin = Input->bDeprecatedExposeInputDefaultAsPin;
				}

				Node->Guid = ParameterNode->Guid;
				Node->CachedInput.Name = ParameterNode->CachedParameter.Name;
				Node->CachedInput.Type = ParameterNode->CachedParameter.Type;
				Node->CachedInput.Category = ParameterNode->CachedParameter.Category;
				Node->CachedInput.Description = ParameterNode->CachedParameter.Description;
				Node->CachedInput.DefaultValue = ParameterNode->CachedParameter.DeprecatedDefaultValue;
				Node->NodePosX = ParameterNode->NodePosX;
				Node->NodePosY = ParameterNode->NodePosY;
				NodeCreator.Finalize();

				const UEdGraphPin* OldOutputPin = ParameterNode->GetOutputPin(0);
				UEdGraphPin* NewOutputPin = Node->GetOutputPin(0);

				if (ensure(OldOutputPin) &&
					ensure(NewOutputPin))
				{
					NewOutputPin->CopyPersistentDataFromOldPin(*OldOutputPin);
				}
			}
			else if (ensure(ParameterNode->Type == EVoxelGraphParameterType_DEPRECATED::Output))
			{
				FGraphNodeCreator<UVoxelGraphNode_Output> NodeCreator(*this);
				UVoxelGraphNode_Output* Node = NodeCreator.CreateNode(false);
				Node->Guid = ParameterNode->Guid;
				Node->CachedOutput.Name = ParameterNode->CachedParameter.Name;
				Node->CachedOutput.Type = ParameterNode->CachedParameter.Type;
				Node->CachedOutput.Category = ParameterNode->CachedParameter.Category;
				Node->CachedOutput.Description = ParameterNode->CachedParameter.Description;
				Node->NodePosX = ParameterNode->NodePosX;
				Node->NodePosY = ParameterNode->NodePosY;
				NodeCreator.Finalize();

				const UEdGraphPin* OldInputPin = ParameterNode->GetInputPin(0);
				UEdGraphPin* NewInputPin = Node->GetInputPin(0);

				if (ensure(OldInputPin) &&
					ensure(NewInputPin))
				{
					NewInputPin->CopyPersistentDataFromOldPin(*OldInputPin);
				}
			}

			ParameterNode->DestroyNode();
		}

		TArray<UVoxelGraphNode_LocalVariableDeclaration*> DeclarationNodes;
		GetNodesOfClass<UVoxelGraphNode_LocalVariableDeclaration>(DeclarationNodes);

		TVoxelSet<FGuid> ParametersWithDeclarationNode;
		for (const UVoxelGraphNode_LocalVariableDeclaration* DeclarationNode : DeclarationNodes)
		{
			ParametersWithDeclarationNode.Add(DeclarationNode->Guid);
		}

		for (const FGuid& Guid : OuterTerminalGraph->GetLocalVariables())
		{
			const FVoxelGraphLocalVariable& LocalVariable = OuterTerminalGraph->FindLocalVariableChecked(Guid);

			if (!ParametersWithDeclarationNode.Contains(Guid))
			{
				FGraphNodeCreator<UVoxelGraphNode_LocalVariableDeclaration> NodeCreator(*this);
				UVoxelGraphNode_LocalVariableDeclaration* Node = NodeCreator.CreateNode(false);
				Node->Guid = Guid;
				Node->CachedLocalVariable = LocalVariable;
				NodeCreator.Finalize();

				LocalVariable.DeprecatedDefaultValue.ApplyToPinDefaultValue(*Node->GetInputPin(0));
			}

			OuterTerminalGraph->UpdateLocalVariable(Guid, [&](FVoxelGraphLocalVariable& InLocalVariable)
			{
				InLocalVariable.DeprecatedDefaultValue = {};
			});
		}
	}

	if (Version < FVersion::AddFeatureScaleAmplitudeToBaseNoises)
	{
		TArray<UVoxelGraphNode_Struct*> StructNodes;
		GetNodesOfClass<UVoxelGraphNode_Struct>(StructNodes);

		const FVoxelPinValue Value = FVoxelPinValue::Make(1.f);

		for (UVoxelGraphNode_Struct* Node : StructNodes)
		{
			if (!Node->Struct)
			{
				continue;
			}

			if (!Node->Struct->IsA<FVoxelNode_PerlinNoise2D>() &&
				!Node->Struct->IsA<FVoxelNode_PerlinNoise3D>() &&
				!Node->Struct->IsA<FVoxelNode_CellularNoise2D>() &&
				!Node->Struct->IsA<FVoxelNode_CellularNoise3D>() &&
				!Node->Struct->IsA<FVoxelNode_TrueDistanceCellularNoise2D>())
			{
				continue;
			}

			Node->ReconstructNode();

			UEdGraphPin* AmplitudePin = Node->FindPin(STATIC_FNAME("Amplitude"), EGPD_Input);
			UEdGraphPin* FeatureScalePin = Node->FindPin(STATIC_FNAME("FeatureScale"), EGPD_Input);

			if (!ensure(AmplitudePin) ||
				!ensure(FeatureScalePin))
			{
				continue;
			}

			Value.ApplyToPinDefaultValue(*AmplitudePin);
			Value.ApplyToPinDefaultValue(*FeatureScalePin);
		}
	}

	if (Version < FVersion::RemoveMacros)
	{
		TArray<UDEPRECATED_VoxelGraphNode_Macro*> MacroNodes;
		GetNodesOfClass<UDEPRECATED_VoxelGraphNode_Macro>(MacroNodes);

		for (UDEPRECATED_VoxelGraphNode_Macro* MacroNode : MacroNodes)
		{
			UVoxelTerminalGraph* MacroTerminalGraph = nullptr;
			if (MacroNode->GraphInterface)
			{
				MacroTerminalGraph = &MacroNode->GraphInterface->GetMainTerminalGraph();
			}

			if (MacroNode->Type == EVoxelGraphMacroType_DEPRECATED::Macro)
			{
				if (!MacroTerminalGraph ||
					!MacroTerminalGraph->IsMainTerminalGraph())
				{
					FGraphNodeCreator<UVoxelGraphNode_CallFunction> NodeCreator(*this);
					UVoxelGraphNode_CallFunction* Node = NodeCreator.CreateNode(false);
					if (MacroTerminalGraph)
					{
						Node->Guid = MacroTerminalGraph->GetGuid();
					}
					Node->NodePosX = MacroNode->NodePosX;
					Node->NodePosY = MacroNode->NodePosY;
					NodeCreator.Finalize();

					for (UEdGraphPin* OldPin : MacroNode->GetAllPins())
					{
						UEdGraphPin* NewPin = Node->FindPin(OldPin->GetFName());
						if (!ensure(NewPin) ||
							!ensure(NewPin->Direction == OldPin->Direction))
						{
							continue;
						}

						NewPin->CopyPersistentDataFromOldPin(*OldPin);
					}
				}
				else
				{
					FGraphNodeCreator<UVoxelGraphNode_CallGraph> NodeCreator(*this);
					UVoxelGraphNode_CallGraph* Node = NodeCreator.CreateNode(false);
					Node->Graph = MacroNode->GraphInterface;
					Node->NodePosX = MacroNode->NodePosX;
					Node->NodePosY = MacroNode->NodePosY;
					NodeCreator.Finalize();

					for (UEdGraphPin* OldPin : MacroNode->GetAllPins())
					{
						UEdGraphPin* NewPin = Node->FindPin(OldPin->GetFName());
						if (!ensure(NewPin) ||
							!ensure(NewPin->Direction == OldPin->Direction))
						{
							continue;
						}

						NewPin->CopyPersistentDataFromOldPin(*OldPin);
					}
				}
			}
			else
			{
				FGraphNodeCreator<UVoxelGraphNode_CallGraphParameter> NodeCreator(*this);
				UVoxelGraphNode_CallGraphParameter* Node = NodeCreator.CreateNode(false);
				Node->BaseGraph = MacroNode->GraphInterface;
				Node->NodePosX = MacroNode->NodePosX;
				Node->NodePosY = MacroNode->NodePosY;
				NodeCreator.Finalize();

				for (UEdGraphPin* OldPin : MacroNode->GetAllPins())
				{
					FName NewName = OldPin->GetFName();
					if (NewName == "Template_Graph")
					{
						NewName = FVoxelGraphConstants::PinName_GraphParameter;
					}

					UEdGraphPin* NewPin = Node->FindPin(NewName);
					if (!ensure(NewPin) ||
						!ensure(NewPin->Direction == OldPin->Direction))
					{
						continue;
					}

					NewPin->CopyPersistentDataFromOldPin(*OldPin);
				}
			}

			MacroNode->DestroyNode();
		}
	}

	if (Version < FVersion::AddClampedLerpNodes)
	{
		TArray<UVoxelGraphNode_Struct*> StructNodes;
		GetNodesOfClass<UVoxelGraphNode_Struct>(StructNodes);

		for (UVoxelGraphNode_Struct* Node : StructNodes)
		{
			if (!Node->Struct)
			{
				continue;
			}

			if (Node->Struct->IsA<FVoxelTemplateNode_Lerp>() ||
				Node->Struct->IsA<FVoxelTemplateNode_SmoothStep>())
			{
				Node->ReconstructNode();

				UEdGraphPin* ClampPin = Node->FindPin(TEXT("Clamp"), EGPD_Input);
				if (!ensure(ClampPin))
				{
					continue;
				}

				FVoxelPinValue::Make(false).ApplyToPinDefaultValue(*ClampPin);
				continue;
			}

			if (Node->Struct->IsA<FVoxelTemplateNode_SafeLerp>())
			{
				TVoxelInstancedStruct<FVoxelNode> OldStruct = Node->Struct;
				Node->Struct = FVoxelTemplateNode_Lerp::StaticStruct();

				for (const FVoxelPin& OldPin : OldStruct->GetPins())
				{
					if (const TSharedPtr<FVoxelPin> NewPin = Node->Struct->FindPin(OldPin.Name))
					{
						if (NewPin->IsPromotable())
						{
							Node->Struct->PromotePin(*NewPin, OldPin.GetType());
						}
					}
				}

				Node->ReconstructNode();

				if (UEdGraphPin* ClampPin = Node->FindPin(TEXT("Clamp"), EGPD_Input))
				{
					FVoxelPinValue::Make(true).ApplyToPinDefaultValue(*ClampPin);
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelEdGraph::NotifyGraphChanged(const FEdGraphEditAction& Action)
{
	VOXEL_FUNCTION_COUNTER();

	Super::NotifyGraphChanged(Action);

	if (Action.Action & (GRAPHACTION_AddNode | GRAPHACTION_RemoveNode))
	{
		GVoxelGraphTracker->NotifyEdGraphChanged(*this);
	}
}

void UVoxelEdGraph::PostEditUndo()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditUndo();

	GVoxelGraphTracker->NotifyEdGraphChanged(*this);
}

void UVoxelEdGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	GVoxelGraphTracker->NotifyEdGraphChanged(*this);
}