// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraphNode_Struct.h"

#include "VoxelExecNode.h"
#include "VoxelGraphVisuals.h"
#include "VoxelGraphToolkit.h"
#include "VoxelGraphMigration.h"
#include "VoxelExposedSeed.h"
#include "VoxelTemplateNode.h"
#include "Nodes/VoxelNode_UFunction.h"

#include "UnrealEdGlobals.h"
#include "SourceCodeNavigation.h"
#include "Editor/UnrealEdEngine.h"
#include "Preferences/UnrealEdOptions.h"

void UVoxelGraphNode_Struct::AllocateDefaultPins()
{
	ON_SCOPE_EXIT
	{
		Super::AllocateDefaultPins();
	};

	if (!Struct.IsValid())
	{
		return;
	}

	CachedName = Struct->GetDisplayName();

	for (const FVoxelPin& Pin : Struct->GetPins())
	{
		UEdGraphPin* GraphPin = CreatePin(
			Pin.bIsInput ? EGPD_Input : EGPD_Output,
			Pin.GetType().GetEdGraphPinType(),
			Pin.Name);

		GraphPin->PinFriendlyName = FText::FromString(Pin.Metadata.DisplayName);
		if (GraphPin->PinFriendlyName.IsEmpty())
		{
			GraphPin->PinFriendlyName = INVTEXT(" ");
		}

		GraphPin->PinToolTip = Pin.Metadata.Tooltip.Get();

		GraphPin->bHidden = Struct->IsPinHidden(Pin);

		InitializeDefaultValue(Pin, *GraphPin);
	}

	// If we only have a single pin hide its name
	if (Pins.Num() == 1 &&
		Pins[0]->Direction == EGPD_Output)
	{
		Pins[0]->PinFriendlyName = INVTEXT(" ");
	}
}

void UVoxelGraphNode_Struct::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (!Struct.IsValid())
	{
		return;
	}

	Struct->PreSerialize();
}

void UVoxelGraphNode_Struct::PostPasteNode()
{
	Super::PostPasteNode();

	if (!Struct.IsValid())
	{
		return;
	}

	Struct->PostSerialize();
}

FText UVoxelGraphNode_Struct::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (!Struct.IsValid())
	{
		return FText::FromString(CachedName);
	}

	if (Struct->GetMetadataContainer().HasMetaData(STATIC_FNAME("Autocast")))
	{
		return INVTEXT("->");
	}

	const FText CompactTitle = Struct->GetMetadataContainer().GetMetaDataText(FBlueprintMetadata::MD_CompactNodeTitle);
	if (!CompactTitle.IsEmpty())
	{
		return CompactTitle;
	}

	return FText::FromString(Struct->GetDisplayName());
}

FLinearColor UVoxelGraphNode_Struct::GetNodeTitleColor() const
{
	if (Struct.IsValid() &&
		Struct->GetMetadataContainer().HasMetaDataHierarchical(STATIC_FNAME("NodeColor")))
	{
		return FVoxelGraphVisuals::GetNodeColor(GetStringMetaDataHierarchical(&Struct->GetMetadataContainer(), STATIC_FNAME("NodeColor")));
	}

	if (Struct.IsValid() &&
		Struct->IsPureNode())
	{
		return GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
	}

	return GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;
}

FText UVoxelGraphNode_Struct::GetTooltipText() const
{
	if (!Struct.IsValid())
	{
		return {};
	}

	return FText::FromString(Struct->GetTooltip());
}

FSlateIcon UVoxelGraphNode_Struct::GetIconAndTint(FLinearColor& OutColor) const
{
	FSlateIcon Icon("EditorStyle", "Kismet.AllClasses.FunctionIcon");
	OutColor = GetDefault<UGraphEditorSettings>()->FunctionCallNodeTitleColor;

	if (!Struct.IsValid())
	{
		return Icon;
	}

	if (Struct->IsPureNode())
	{
		OutColor = GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
	}

	if (Struct->GetMetadataContainer().HasMetaDataHierarchical("NativeMakeFunc"))
	{
		Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.MakeStruct_16x");
		OutColor = FLinearColor::White;
	}

	if (Struct->GetMetadataContainer().HasMetaDataHierarchical("NativeBreakFunc"))
	{
		Icon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.BreakStruct_16x");
		OutColor = FLinearColor::White;
	}

	if (Struct->GetMetadataContainer().HasMetaDataHierarchical("NodeIcon"))
	{
		Icon = FVoxelGraphVisuals::GetNodeIcon(GetStringMetaDataHierarchical(&Struct->GetMetadataContainer(), "NodeIcon"));
	}

	if (Struct->GetMetadataContainer().HasMetaDataHierarchical("NodeIconColor"))
	{
		OutColor = FVoxelGraphVisuals::GetNodeColor(GetStringMetaDataHierarchical(&Struct->GetMetadataContainer(), "NodeIconColor"));
	}

	return Icon;
}

bool UVoxelGraphNode_Struct::IsCompact() const
{
	if (!Struct.IsValid())
	{
		return {};
	}

	return
		Struct->GetMetadataContainer().HasMetaData(STATIC_FNAME("Autocast")) ||
		Struct->GetMetadataContainer().HasMetaData(FBlueprintMetadata::MD_CompactNodeTitle);
}

bool UVoxelGraphNode_Struct::GetOverlayInfo(FString& Type, FString& Tooltip, FString& Color)
{
	if (!Struct.IsValid())
	{
		return false;
	}

	const UStruct& MetadataContainer = Struct->GetMetadataContainer();
	if (!MetadataContainer.HasMetaDataHierarchical("OverlayTooltip") &&
		!MetadataContainer.HasMetaDataHierarchical("OverlayType") &&
		!MetadataContainer.HasMetaDataHierarchical("OverlayColor"))
	{
		return false;
	}

	if (MetadataContainer.HasMetaDataHierarchical("OverlayTooltip"))
	{
		Tooltip = GetStringMetaDataHierarchical(&MetadataContainer, "OverlayTooltip");
	}

	if (MetadataContainer.HasMetaDataHierarchical("OverlayType"))
	{
		Type = GetStringMetaDataHierarchical(&MetadataContainer, "OverlayType");
	}
	else
	{
		Type = "Warning";
	}

	if (MetadataContainer.HasMetaDataHierarchical("OverlayColor"))
	{
		Color = GetStringMetaDataHierarchical(&MetadataContainer, "OverlayColor");
	}

	return true;
}

bool UVoxelGraphNode_Struct::ShowAsPromotableWildcard(const UEdGraphPin& Pin) const
{
	if (!Struct.IsValid() ||
		Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	const TSharedPtr<const FVoxelPin> StructPin = Struct->FindPin(Pin.PinName);
	if (!ensure(StructPin))
	{
		return false;
	}

	if (!StructPin->IsPromotable() ||
		!Struct->ShowPromotablePinsAsWildcards())
	{
		return false;
	}

	return true;
}

bool UVoxelGraphNode_Struct::IsPinOptional(const UEdGraphPin& Pin) const
{
	if (!Struct.IsValid() ||
		Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	const TSharedPtr<const FVoxelPin> StructPin = Struct->FindPin(Pin.PinName);
	if (!ensureVoxelSlow(StructPin))
	{
		return {};
	}

	return StructPin->Metadata.bOptionalPin;
}

bool UVoxelGraphNode_Struct::ShouldHidePinDefaultValue(const UEdGraphPin& Pin) const
{
	if (!Struct.IsValid() ||
		Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	const TSharedPtr<const FVoxelPin> StructPin = Struct->FindPin(Pin.PinName);
	if (!ensureVoxelSlow(StructPin))
	{
		return {};
	}

	return StructPin->Metadata.bNoDefault;
}

FName UVoxelGraphNode_Struct::GetPinCategory(const UEdGraphPin& Pin) const
{
	if (!Struct.IsValid())
	{
		return {};
	}

	const TSharedPtr<const FVoxelPin> StructPin = Struct->FindPin(Pin.PinName);
	if (!ensure(StructPin))
	{
		return {};
	}

	return *StructPin->Metadata.Category;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<IVoxelNodeDefinition> UVoxelGraphNode_Struct::GetNodeDefinition()
{
	if (!Struct)
	{
		return MakeVoxelShared<IVoxelNodeDefinition>();
	}

	const TSharedRef<FVoxelNodeDefinition> NodeDefinition = Struct->GetNodeDefinition();
	NodeDefinition->Initialize(*this);
	return MakeVoxelShared<FVoxelNodeDefinition_Struct>(*this, NodeDefinition);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphNode_Struct::CanRemovePin_ContextMenu(const UEdGraphPin& Pin) const
{
	if (!Struct.IsValid() ||
		Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	return ConstCast(this)->GetNodeDefinition()->CanRemoveSelectedPin(Pin.PinName);
}

void UVoxelGraphNode_Struct::RemovePin_ContextMenu(UEdGraphPin& Pin)
{
	GetNodeDefinition()->RemoveSelectedPin(Pin.PinName);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphNode_Struct::CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const
{
	if (Pin.ParentPin ||
		Pin.bOrphanedPin)
	{
		return false;
	}

	const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(Pin.PinName);
	if (!ensure(VoxelPin) ||
		!VoxelPin->IsPromotable())
	{
		return false;
	}

	OutTypes = Struct->GetPromotionTypes(*VoxelPin);
	return true;
}

FString UVoxelGraphNode_Struct::GetPinPromotionWarning(const UEdGraphPin& Pin, const FVoxelPinType& NewType) const
{
	const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(Pin.PinName);
	if (!ensure(VoxelPin) ||
		!VoxelPin->IsPromotable())
	{
		return {};
	}

	return Struct->GetPinPromotionWarning(*VoxelPin, NewType);
}

void UVoxelGraphNode_Struct::PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType)
{
	Modify();

	const TSharedPtr<FVoxelPin> VoxelPin = Struct->FindPin(Pin.PinName);
	if (!ensure(VoxelPin) ||
		!ensure(VoxelPin->IsPromotable()))
	{
		return;
	}

	Struct->PromotePin(*VoxelPin, NewType);

	ReconstructFromVoxelNode();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraphNode_Struct::TryMigratePin(UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	if (!Super::TryMigratePin(OldPin, NewPin))
	{
		return false;
	}

	if (Struct)
	{
		if (const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(NewPin->PinName))
		{
			if (VoxelPin->Metadata.bShowInDetail)
			{
				const FString DefaultValueString = Struct->GetPinDefaultValue(*VoxelPin.Get());

				FVoxelPinValue DefaultValue(VoxelPin->GetType().GetPinDefaultValueType());
				ensure(DefaultValue.ImportFromString(DefaultValueString));
				DefaultValue.ApplyToPinDefaultValue(*NewPin);
			}
		}
	}

	return true;
}

void UVoxelGraphNode_Struct::TryMigrateDefaultValue(const UEdGraphPin* OldPin, UEdGraphPin* NewPin) const
{
	if (const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(NewPin->PinName))
	{
		if (VoxelPin->Metadata.bShowInDetail)
		{
			return;
		}
	}

	Super::TryMigrateDefaultValue(OldPin, NewPin);
}

FName UVoxelGraphNode_Struct::GetMigratedPinName(const FName PinName) const
{
	if (!Struct)
	{
		return PinName;
	}

	UObject* Outer;
	if (Struct->IsA<FVoxelNode_UFunction>())
	{
		Outer = Struct.Get<FVoxelNode_UFunction>().GetFunction();
	}
	else
	{
		Outer = Struct.GetScriptStruct();
	}

	if (!Outer)
	{
		return PinName;
	}

	const FName MigratedName = GVoxelGraphMigration->FindNewPinName(Outer, PinName);
	if (MigratedName.IsNone())
	{
		return PinName;
	}

	return MigratedName;
}

void UVoxelGraphNode_Struct::PreReconstructNode()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PreReconstructNode();

	if (!Struct)
	{
		return;
	}

	for (const UEdGraphPin* Pin : Pins)
	{
		if (Pin->bOrphanedPin ||
			Pin->ParentPin ||
			Pin->LinkedTo.Num() > 0 ||
			!FVoxelPinType(Pin->PinType).IsValid() ||
			!FVoxelPinType(Pin->PinType).HasPinDefaultValue())
		{
			return;
		}

		const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(Pin->PinName);
		if (!VoxelPin)
		{
			// Orphaned
			continue;
		}

		const FVoxelPinValue DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*Pin);
		GetNodeDefinition()->OnPinDefaultValueChanged(Pin->PinName, DefaultValue);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Struct::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if (const TSharedPtr<const FVoxelPin> VoxelPin = Struct->FindPin(Pin->PinName))
	{
		const FVoxelPinValue DefaultValue = FVoxelPinValue::MakeFromPinDefaultValue(*Pin);

		if (VoxelPin->Metadata.bShowInDetail)
		{
			Struct->UpdatePropertyBoundDefaultValue(*VoxelPin.Get(), DefaultValue);
		}

		if (ensure(Pin->LinkedTo.Num() == 0) &&
			GetNodeDefinition()->OnPinDefaultValueChanged(Pin->PinName, DefaultValue))
		{
			const TSharedPtr<FVoxelGraphToolkit> Toolkit = GetToolkit();
			if (ensure(Toolkit))
			{
				Toolkit->AddNodeToReconstruct(this);
			}
		}
	}

	Super::PinDefaultValueChanged(Pin);
}

void UVoxelGraphNode_Struct::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (Struct)
	{
		Struct->PostEditChangeProperty(PropertyChangedEvent);
	}

	if (const FProperty* Property = PropertyChangedEvent.MemberProperty)
	{
		if (Property->GetFName() == GET_MEMBER_NAME_STATIC(UVoxelGraphNode_Struct, Struct))
		{
			ReconstructFromVoxelNode();
		}
	}
}

bool UVoxelGraphNode_Struct::CanJumpToDefinition() const
{
	return true;
}

void UVoxelGraphNode_Struct::JumpToDefinition() const
{
	if (!ensure(GUnrealEd) ||
		!GUnrealEd->GetUnrealEdOptions()->IsCPPAllowed())
	{
		return;
	}

	if (!Struct)
	{
		return;
	}

	if (const FVoxelNode_UFunction* FunctionStruct = Struct->As<FVoxelNode_UFunction>())
	{
		if (FSourceCodeNavigation::CanNavigateToFunction(FunctionStruct->GetFunction()))
		{
			FSourceCodeNavigation::NavigateToFunction(FunctionStruct->GetFunction());
		}
		return;
	}

	FString RelativeFilePath;
	if (!FSourceCodeNavigation::FindClassSourcePath(Struct->GetStruct(), RelativeFilePath) ||
		IFileManager::Get().FileSize(*RelativeFilePath) == -1)
	{
		return;
	}

	int32 Line = -1;
	if (FindVoxelNodeComputeLine(Struct->GetStruct(), Line))
	{
		FSourceCodeNavigation::OpenSourceFile(FPaths::ConvertRelativePathToFull(RelativeFilePath), Line);
		return;
	}

	FString ModuleName;
	const bool bModuleNameFound = INLINE_LAMBDA
	{
		if (!Struct->GetStruct())
		{
			return false;
		}

		const UPackage* Package = Struct->GetStruct()->GetPackage();
		if (!Package)
		{
			return false;
		}

		const FName ShotPackageName = FPackageName::GetShortFName(Package->GetFName());
		if (!FModuleManager::Get().IsModuleLoaded(ShotPackageName))
		{
			return false;
		}

		FModuleStatus ModuleStatus;
		if(!ensure(FModuleManager::Get().QueryModule(ShotPackageName, ModuleStatus)))
		{
			return false;
		}

		ModuleName = FPaths::GetBaseFilename(ModuleStatus.FilePath);
		return true;
	};

	if (!bModuleNameFound)
	{
		return;
	}

	FString Function = "CompileCompute";
	if (Struct->IsA<FVoxelExecNode>())
	{
		Function = "CreateExecRuntime";
	}
	else if (Struct->IsA<FVoxelTemplateNode>())
	{
		Function = "GetNodeDefinition";
	}

	const FString FunctionName = Struct->GetStruct()->GetStructCPPName() + "::" + Function;
	FSourceCodeNavigation::NavigateToFunctionSourceAsync(FunctionName, ModuleName, false);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraphNode_Struct::ReconstructFromVoxelNode()
{
	ReconstructNode(false);
}

void UVoxelGraphNode_Struct::InitializeDefaultValue(const FVoxelPin& VoxelPin, UEdGraphPin& GraphPin)
{
	const FVoxelPinType Type(GraphPin.PinType);
	ensure(Type == VoxelPin.GetType());

	if (!Type.HasPinDefaultValue())
	{
		ensure(!GraphPin.DefaultObject);
		ensure(GraphPin.DefaultValue.IsEmpty());
		ensure(GraphPin.AutogeneratedDefaultValue.IsEmpty());
		return;
	}

	const FVoxelPinType InnerExposedType = Type.GetPinDefaultValueType();
	FString DefaultValueString = Struct->GetPinDefaultValue(VoxelPin);

	if (InnerExposedType.Is<FVoxelExposedSeed>())
	{
		ensure(DefaultValueString.IsEmpty());

		FVoxelExposedSeed NewSeed;
		if (IsRunningCommandlet())
		{
			// Be deterministic when generating docs
			const FString Hash = GetNodeTitle(ENodeTitleType::FullTitle).ToString() + "." + VoxelPin.Name.ToString();
			NewSeed.Randomize(FCrc::StrCrc32(*Hash));
		}
		else
		{
			NewSeed.Randomize();
		}

		const FVoxelPinValue DefaultValue = FVoxelPinValue::Make(NewSeed);
		Struct->UpdatePropertyBoundDefaultValue(VoxelPin, DefaultValue);

		GraphPin.ResetDefaultValue();
		GraphPin.DefaultValue = DefaultValue.ExportToString();
		GraphPin.AutogeneratedDefaultValue = {};
		return;
	}

	if (InnerExposedType.IsObject())
	{
		if (DefaultValueString.IsEmpty())
		{
			ensure(!GraphPin.DefaultObject);
			ensure(GraphPin.DefaultValue.IsEmpty());
			ensure(GraphPin.AutogeneratedDefaultValue.IsEmpty());
			return;
		}

		UObject* Object = FSoftObjectPtr(DefaultValueString).LoadSynchronous();
		if (!ensure(Object))
		{
			GraphPin.ResetDefaultValue();
			GraphPin.AutogeneratedDefaultValue = {};
			return;
		}

		GraphPin.ResetDefaultValue();
		GraphPin.DefaultObject = Object;
		GraphPin.AutogeneratedDefaultValue = Object->GetPathName();
		return;
	}

	// Try to fixup the default value
	if (DefaultValueString.IsEmpty())
	{
		FVoxelPinValue Value(InnerExposedType);
		DefaultValueString = Value.ExportToString();
	}

	{
		FVoxelPinValue Value(InnerExposedType);
		ensure(Value.ImportFromString(DefaultValueString));
	}

	GraphPin.ResetDefaultValue();
	GraphPin.DefaultValue = DefaultValueString;
	GraphPin.AutogeneratedDefaultValue = DefaultValueString;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition_Struct::GetInputs() const
{
	return NodeDefinition->GetInputs();
}

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition_Struct::GetOutputs() const
{
	return NodeDefinition->GetOutputs();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString FVoxelNodeDefinition_Struct::GetAddPinLabel() const
{
	return NodeDefinition->GetAddPinLabel();
}

FString FVoxelNodeDefinition_Struct::GetAddPinTooltip() const
{
	return NodeDefinition->GetAddPinTooltip();
}

FString FVoxelNodeDefinition_Struct::GetRemovePinTooltip() const
{
	return NodeDefinition->GetRemovePinTooltip();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelNodeDefinition_Struct::CanAddInputPin() const
{
	return NodeDefinition->CanAddInputPin();
}

void FVoxelNodeDefinition_Struct::AddInputPin()
{
	FScope Scope(this, "Add Input Pin");
	NodeDefinition->AddInputPin();
}

bool FVoxelNodeDefinition_Struct::CanRemoveInputPin() const
{
	return NodeDefinition->CanRemoveInputPin();
}

void FVoxelNodeDefinition_Struct::RemoveInputPin()
{
	FScope Scope(this, "Remove Input Pin");
	NodeDefinition->RemoveInputPin();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelNodeDefinition_Struct::Variadic_CanAddPinTo(const FName VariadicPinName) const
{
	return NodeDefinition->Variadic_CanAddPinTo(VariadicPinName);
}

FName FVoxelNodeDefinition_Struct::Variadic_AddPinTo(const FName VariadicPinName)
{
	FScope Scope(this, "Add To Category");
	return NodeDefinition->Variadic_AddPinTo(VariadicPinName);
}

bool FVoxelNodeDefinition_Struct::Variadic_CanRemovePinFrom(const FName VariadicPinName) const
{
	return NodeDefinition->Variadic_CanRemovePinFrom(VariadicPinName);
}

void FVoxelNodeDefinition_Struct::Variadic_RemovePinFrom(const FName VariadicPinName)
{
	FScope Scope(this, "Remove From Category");
	NodeDefinition->Variadic_RemovePinFrom(VariadicPinName);
}

bool FVoxelNodeDefinition_Struct::CanRemoveSelectedPin(const FName PinName) const
{
	return NodeDefinition->CanRemoveSelectedPin(PinName);
}

void FVoxelNodeDefinition_Struct::RemoveSelectedPin(const FName PinName)
{
	FScope Scope(this, "Remove " + PinName.ToString() + " Pin");
	NodeDefinition->RemoveSelectedPin(PinName);
}

void FVoxelNodeDefinition_Struct::InsertPinBefore(const FName PinName)
{
	FScope Scope(this, "Insert Pin Before " + PinName.ToString());
	NodeDefinition->InsertPinBefore(PinName);
}

void FVoxelNodeDefinition_Struct::DuplicatePin(const FName PinName)
{
	FScope Scope(this, "Duplicate Pin " + PinName.ToString());
	NodeDefinition->DuplicatePin(PinName);
}

bool FVoxelNodeDefinition_Struct::IsPinVisible(const UEdGraphPin* Pin, const UEdGraphNode* GraphNode)
{
	return NodeDefinition->IsPinVisible(Pin, GraphNode);
}

bool FVoxelNodeDefinition_Struct::OnPinDefaultValueChanged(const FName PinName, const FVoxelPinValue& NewDefaultValue)
{
	return NodeDefinition->OnPinDefaultValueChanged(PinName, NewDefaultValue);
}

void FVoxelNodeDefinition_Struct::ExposePin(const FName PinName)
{
	FScope Scope(this, "Expose Pin " + PinName.ToString());
	NodeDefinition->ExposePin(PinName);
}