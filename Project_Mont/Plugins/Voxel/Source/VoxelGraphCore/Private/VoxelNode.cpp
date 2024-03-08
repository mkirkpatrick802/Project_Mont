// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelNode.h"
#include "VoxelGraph.h"
#include "VoxelExecNode.h"
#include "VoxelQueryCache.h"
#include "VoxelTemplateNode.h"
#include "VoxelSourceParser.h"
#include "VoxelGraphEvaluator.h"
#include "VoxelCompilationGraph.h"

DEFINE_UNIQUE_VOXEL_ID(FVoxelPinRuntimeId);
DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelNodeRuntime);

TMap<FName, FVoxelNodeComputePtr> GVoxelNodeOutputNameToCompute;
#if WITH_EDITOR
TMap<const UScriptStruct*, int32> GVoxelNodeToLine;
#endif

void RegisterVoxelNodeComputePtr(
	const UScriptStruct* Node,
	const FName PinName,
	const FVoxelNodeComputePtr Ptr,
	const int32 Line)
{
	VOXEL_FUNCTION_COUNTER();

	static const TArray<UScriptStruct*> NodeStructs = GetDerivedStructs<FVoxelNode>();

	for (const UScriptStruct* ChildNode : NodeStructs)
	{
		if (!ChildNode->IsChildOf(Node))
		{
			continue;
		}

		const FName Name(ChildNode->GetStructCPPName() + "." + PinName.ToString());

		ensure(!GVoxelNodeOutputNameToCompute.Contains(Name));
		GVoxelNodeOutputNameToCompute.Add(Name, Ptr);
	}

#if WITH_EDITOR
	GVoxelNodeToLine.FindOrAdd(Node, Line);
#endif
}

#if WITH_EDITOR
bool FindVoxelNodeComputeLine(const UScriptStruct* Node, int32& OutLine)
{
	if (const int32* LinePtr = GVoxelNodeToLine.Find(Node))
	{
		OutLine = *LinePtr;
		return true;
	}

	return false;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
VOXEL_RUN_ON_STARTUP_GAME(FixupVoxelNodes)
{
	VOXEL_FUNCTION_COUNTER();

	for (UScriptStruct* Struct : GetDerivedStructs<FVoxelNode>())
	{
		if (Struct->HasMetaData(STATIC_FNAME("Abstract")) ||
			Struct->HasMetaData(STATIC_FNAME("Internal")))
		{
			continue;
		}

		if (Struct->GetPathName().StartsWith(TEXT("/Script/Voxel")))
		{
			const UStruct* Parent = Struct;
			while (Parent->GetSuperStruct() != FVoxelNode::StaticStruct())
			{
				Parent = Parent->GetSuperStruct();
			}

			if (Parent == FVoxelTemplateNode::StaticStruct())
			{
				ensure(Struct->GetStructCPPName().StartsWith("FVoxelTemplateNode_"));
			}
			else if (Parent == FVoxelExecNode::StaticStruct())
			{
				ensure(Struct->GetStructCPPName().StartsWith("FVoxelExecNode_"));
			}
			else
			{
				ensure(Struct->GetStructCPPName().StartsWith("FVoxelNode_"));
			}
		}

		TArray<FString> Array;
		Struct->GetName().ParseIntoArray(Array, TEXT("_"));

		const FString DefaultName = FName::NameToDisplayString(Struct->GetName(), false);
		const FString FixedName = FName::NameToDisplayString(Array.Last(), false);

		if (Struct->GetDisplayNameText().ToString() == DefaultName)
		{
			Struct->SetMetaData("DisplayName", *FixedName);
		}

		FString Tooltip = Struct->GetToolTipText().ToString();
		if (Tooltip.RemoveFromStart("Voxel Node ") ||
			Tooltip.RemoveFromStart("Voxel Exec Node "))
		{
			Struct->SetMetaData("Tooltip", *Tooltip);
		}
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelNodeRuntime::Get(
	const FVoxelPinRef& Pin,
	const FVoxelQuery& Query) const
{
	const FPinData& PinData = GetPinData(Pin);
	checkVoxelSlow(PinData.bIsInput);
	ensureVoxelSlow(!PinData.Metadata.bConstantPin);

	FVoxelFutureValue Value = (*PinData.Compute)(Query.EnterScope(GetNodeRef(), Pin));
	checkVoxelSlow(Value.IsValid());
	checkVoxelSlow(
		PinData.Type.CanBeCastedTo(Value.GetParentType()) ||
		Value.GetParentType().CanBeCastedTo(PinData.Type));
	return Value;
}

TVoxelArray<FVoxelFutureValue> FVoxelNodeRuntime::Get(
	const FVoxelVariadicPinRef& VariadicPin,
	const FVoxelQuery& Query) const
{
	const TVoxelArray<FName>& PinNames = VariadicPinNameToPinNames[VariadicPin];

	TVoxelArray<FVoxelFutureValue> Result;
	Result.Reserve(PinNames.Num());
	for (const FName Pin : PinNames)
	{
		Result.Add(Get(FVoxelPinRef(Pin), Query));
	}
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<const FVoxelComputeValue> FVoxelNodeRuntime::GetCompute(
	const FVoxelPinRef& Pin,
	const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance) const
{
	const TSharedPtr<FPinData> PinData = NameToPinData.FindRef(Pin);
	checkVoxelSlow(PinData);
	ensureVoxelSlow(!PinData->Metadata.bConstantPin);

	return MakeVoxelShared<FVoxelComputeValue>([TerminalGraphInstance, Compute = PinData->Compute.ToSharedRef()](const FVoxelQuery& InQuery)
	{
		const FVoxelQuery NewQuery = InQuery.MakeNewQuery(TerminalGraphInstance);
		const FVoxelQueryScope Scope(NewQuery);
		return (*Compute)(NewQuery);
	});
}

FVoxelDynamicValueFactory FVoxelNodeRuntime::MakeDynamicValueFactory(const FVoxelPinRef& Pin) const
{
	const TSharedPtr<FPinData> PinData = NameToPinData.FindRef(Pin);
	checkVoxelSlow(PinData);
	ensureVoxelSlow(!PinData->Metadata.bConstantPin);
	// If we're not a VirtualPin the executor won't be kept alive by Compute
	checkVoxelSlow(PinData->Metadata.bVirtualPin);
	return FVoxelDynamicValueFactory(PinData->Compute.ToSharedRef(), PinData->Type, PinData->StatName);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinRuntimeId GetVoxelPinRuntimeId(const FVoxelGraphPinRef& PinRef)
{
	static FVoxelCriticalSection CriticalSection;
	VOXEL_SCOPE_LOCK(CriticalSection);

	static TMap<FVoxelGraphPinRef, FVoxelPinRuntimeId> PinRefToId;

	FVoxelPinRuntimeId& Id = PinRefToId.FindOrAdd(PinRef);
	if (!Id.IsValid())
	{
		Id = FVoxelPinRuntimeId::New();
	}
	return Id;
}

FVoxelNodeRuntime::FPinData::FPinData(
	const FVoxelPinType& Type,
	const bool bIsInput,
	const FName StatName,
	const FVoxelGraphPinRef& PinRef,
	const FVoxelPinMetadataFlags& Metadata)
	: Type(Type)
	, bIsInput(bIsInput)
	, StatName(StatName)
	, PinId(GetVoxelPinRuntimeId(PinRef))
	, Metadata(Metadata)
{
	ensure(!Type.IsWildcard());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_VOXEL_INSTANCE_COUNTER(FVoxelNode);
DEFINE_VOXEL_MEMORY_STAT(STAT_VoxelNodes);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelNode& FVoxelNode::operator=(const FVoxelNode& Other)
{
	VOXEL_SCOPE_COUNTER_FORMAT("FVoxelNode::operator= %s", *GetStruct()->GetName());

	check(GetStruct() == Other.GetStruct());
	ensure(!NodeRuntime);
	ensure(!Other.NodeRuntime);

#if WITH_EDITOR
	ExposedPins = Other.ExposedPins;
	ExposedPinValues = Other.ExposedPinValues;
#endif

	FlushDeferredPins();
	Other.FlushDeferredPins();

	PrivateNameToPinBackup.Reset();
	PrivateNameToPin.Reset();
	PrivateNameToVariadicPin.Reset();
	PrivatePinsOrder.Reset();

	SortOrderCounter = Other.SortOrderCounter;

	TVoxelArray<FDeferredPin> Pins = Other.PrivateNameToPinBackup.ValueArray();

	// Register variadic pins first
	Pins.Sort([](const FDeferredPin& A, const FDeferredPin& B)
	{
		return A.IsVariadicRoot() > B.IsVariadicRoot();
	});

	for (const FDeferredPin& Pin : Pins)
	{
		RegisterPin(Pin, false);
	}

	PrivatePinsOrder.Append(Other.PrivatePinsOrder);

	LoadSerializedData(Other.GetSerializedData());
	UpdateStats();

	return *this;
}

int64 FVoxelNode::GetAllocatedSize() const
{
	int64 AllocatedSize = GetStruct()->GetStructureSize();

	AllocatedSize += DeferredPins.GetAllocatedSize();
	AllocatedSize += PrivateNameToPinBackup.GetAllocatedSize();
	AllocatedSize += PrivatePinsOrder.GetAllocatedSize();
	AllocatedSize += PrivateNameToPin.GetAllocatedSize();
	AllocatedSize += PrivateNameToPin.Num() * sizeof(FVoxelPin);
	AllocatedSize += PrivateNameToVariadicPin.GetAllocatedSize();
#if WITH_EDITOR
	AllocatedSize += ExposedPinValues.GetAllocatedSize();
	AllocatedSize += ExposedPins.GetAllocatedSize();
#endif
	AllocatedSize += ReturnToPoolFuncs.GetAllocatedSize();

	for (const auto& It : PrivateNameToVariadicPin)
	{
		AllocatedSize += It.Value->Pins.GetAllocatedSize();
	}

	return AllocatedSize;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
UStruct& FVoxelNode::GetMetadataContainer() const
{
	return *GetStruct();
}

FString FVoxelNode::GetCategory() const
{
	FString Category;
	ensureMsgf(
		GetMetadataContainer().GetStringMetaDataHierarchical(STATIC_FNAME("Category"), &Category) &&
		!Category.IsEmpty(),
		TEXT("%s is missing a Category"),
		*GetStruct()->GetStructCPPName());
	return Category;
}

FString FVoxelNode::GetDisplayName() const
{
	if (GetMetadataContainer().HasMetaData(STATIC_FNAME("Autocast")) &&
		ensure(PrivatePinsOrder.Num() > 0) &&
		ensure(FindPin(PrivatePinsOrder[0])))
	{
		ensure(!GetMetadataContainer().HasMetaData(STATIC_FNAME("DisplayName")));

		const FString FromType = FindPin(PrivatePinsOrder[0])->GetType().GetInnerType().ToString();
		const FString ToType = GetUniqueOutputPin().GetType().GetInnerType().ToString();
		{
			FString ExpectedName = FromType + "To" + ToType;
			ExpectedName.ReplaceInline(TEXT(" "), TEXT(""));
			ensure(GetMetadataContainer().GetName() == ExpectedName);
		}
		return "To " + ToType + " (" + FromType + ")";
	}

	return GetMetadataContainer().GetDisplayNameText().ToString();
}

FString FVoxelNode::GetTooltip() const
{
	if (GetMetadataContainer().HasMetaData(STATIC_FNAME("Autocast")) &&
		ensure(PrivatePinsOrder.Num() > 0) &&
		ensure(FindPin(PrivatePinsOrder[0])))
	{
		ensure(!GetMetadataContainer().HasMetaData(STATIC_FNAME("Tooltip")));
		ensure(!GetMetadataContainer().HasMetaData(STATIC_FNAME("ShortTooltip")));

		const FString FromType = FindPin(PrivatePinsOrder[0])->GetType().GetInnerType().ToString();
		const FString ToType = GetUniqueOutputPin().GetType().GetInnerType().ToString();

		return "Cast from " + FromType + " to " + ToType;
	}

	return GetMetadataContainer().GetToolTipText().ToString();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::ReturnToPool()
{
	for (const auto& It : NodeRuntime->NameToPinData)
	{
		if (It.Value->bIsInput)
		{
			It.Value->Compute = nullptr;
		}
	}

	for (const FReturnToPoolFunc& ReturnToPoolFunc : ReturnToPoolFuncs)
	{
		(this->*ReturnToPoolFunc)();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelComputeValue FVoxelNode::CompileCompute(const FName PinName) const
{
	const FName Name = FName(GetStruct()->GetStructCPPName() + "." + PinName.ToString());
	if (!GVoxelNodeOutputNameToCompute.Contains(Name))
	{
		return nullptr;
	}

	const FVoxelNodeComputePtr Ptr = GVoxelNodeOutputNameToCompute.FindChecked(Name);
	const FVoxelNodeRuntime::FPinData& PinData = GetNodeRuntime().GetPinData(PinName);

	if (!ensure(!PinData.Type.IsWildcard()))
	{
		return {};
	}

	return [this, Ptr, &PinData](const FVoxelQuery& Query) -> FVoxelFutureValue
	{
		checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));
		ON_SCOPE_EXIT
		{
			checkVoxelSlow(FVoxelTaskReferencer::Get().IsReferenced(this));
		};
		FVoxelQueryScope Scope(Query);

		// No caching for call nodes
		if (GetNodeRuntime().IsCallNode())
		{
			return
				MakeVoxelTaskImpl(PinData.StatName)
				.Execute(PinData.Type, [this, Ptr, Query]
				{
					return (*Ptr)(*this, Query.EnterScope(*this));
				});
		}

		FVoxelQueryCache::FEntry& Entry = Query.GetQueryCache().FindOrAddEntry(PinData.PinId);

		VOXEL_SCOPE_LOCK(Entry.CriticalSection);

		if (!Entry.Value.IsValid())
		{
			// Always wrap in a task to sanitize the values,
			// otherwise errors propagate to other nodes and are impossible to track down
			Entry.Value =
				MakeVoxelTaskImpl(PinData.StatName)
				.Execute(PinData.Type, [this, Ptr, Query]
				{
					return (*Ptr)(*this, Query.EnterScope(*this));
				});
		}
		return Entry.Value;
	};
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint32 FVoxelNode::GetNodeHash() const
{
	ensure(!NodeRuntime.IsValid());

	// Ensure properties are up to date
	ConstCast(this)->PreSerialize();

	uint64 Hash = FVoxelUtilities::MurmurHash(GetStruct());
	int32 Index = 0;
	// Only hash our own properties, handling child structs properties is too messy/not required
	for (const FProperty& Property : GetStructProperties(StaticStructFast<FVoxelNode>()))
	{
		const uint32 PropertyHash = FVoxelUtilities::HashProperty(Property, Property.ContainerPtrToValuePtr<void>(this));
		Hash ^= FVoxelUtilities::MurmurHash(PropertyHash, Index++);
	}
	return Hash;
}

bool FVoxelNode::IsNodeIdentical(const FVoxelNode& Other) const
{
	if (GetStruct() != Other.GetStruct())
	{
		return false;
	}

	// Ensure properties are up to date
	// (needed for pin arrays)
	ConstCast(this)->PreSerialize();
	ConstCast(Other).PreSerialize();

	return GetStruct()->CompareScriptStruct(this, &Other, PPF_None);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::PreSerialize()
{
	Super::PreSerialize();

	Version = FVersion::LatestVersion;

	SerializedDataProperty = GetSerializedData();
}

void FVoxelNode::PostSerialize()
{
	Super::PostSerialize();

	if (Version < FVersion::AddIsValid)
	{
		ensure(!SerializedDataProperty.bIsValid);
		SerializedDataProperty.bIsValid = true;
	}
	ensure(SerializedDataProperty.bIsValid);

	LoadSerializedData(SerializedDataProperty);
	SerializedDataProperty = {};

	UpdateStats();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
TSharedRef<FVoxelNodeDefinition> FVoxelNode::GetNodeDefinition()
{
	return MakeVoxelShared<FVoxelNodeDefinition>(*this);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
FVoxelPinTypeSet FVoxelNode::GetPromotionTypes(const FVoxelPin& Pin) const
{
	if (!ensure(Pin.IsPromotable()))
	{
		return {};
	}

	if (EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::TemplatePin))
	{
		// GetPromotionTypes should be overridden by child nodes
		ensure(!Pin.GetType().IsWildcard());

		FVoxelPinTypeSet Types;
		Types.Add(Pin.GetType().GetInnerType());
		Types.Add(Pin.GetType().GetBufferType());
		return Types;
	}

	ensure(Pin.BaseType.IsWildcard());
	return FVoxelPinTypeSet::All();
}

void FVoxelNode::PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	if (!ensure(Pin.IsPromotable()) ||
		!ensure(GetPromotionTypes(Pin).Contains(NewType)))
	{
		return;
	}

	if (EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::TemplatePin))
	{
		ensure(NewType.GetInnerType() == Pin.GetType().GetInnerType());

		for (FVoxelPin& OtherPin : GetPins())
		{
			if (!EnumHasAllFlags(OtherPin.Flags, EVoxelPinFlags::TemplatePin))
			{
				continue;
			}

			if (NewType.IsBuffer())
			{
				OtherPin.SetType(OtherPin.GetType().GetBufferType());
			}
			else
			{
				OtherPin.SetType(OtherPin.GetType().GetInnerType());
			}
		}
	}

	ensure(Pin.BaseType.IsWildcard());
	Pin.SetType(NewType);
}
#endif

void FVoxelNode::PromotePin_Runtime(FVoxelPin& Pin, const FVoxelPinType& NewType)
{
	// For convenience
	if (Pin.GetType() == NewType)
	{
		return;
	}

#if WITH_EDITOR
	ensure(GetPromotionTypes(Pin).Contains(NewType));
#endif

	if (!ensure(Pin.IsPromotable()))
	{
		return;
	}

	// If this fails, PromotePin_Runtime needs to be overriden
	ensure(Pin.GetType().IsWildcard() || EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::TemplatePin));

	if (Pin.GetType().IsWildcard())
	{
		Pin.SetType(NewType);
	}

	if (!EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::TemplatePin))
	{
		return;
	}

	ensure(NewType.GetInnerType() == Pin.GetType().GetInnerType());

	for (FVoxelPin& OtherPin : GetPins())
	{
		if (!EnumHasAllFlags(OtherPin.Flags, EVoxelPinFlags::TemplatePin))
		{
			continue;
		}

		if (NewType.IsBuffer())
		{
			OtherPin.SetType(OtherPin.GetType().GetBufferType());
		}
		else
		{
			OtherPin.SetType(OtherPin.GetType().GetInnerType());
		}
	}
}

TOptional<bool> FVoxelNode::AreTemplatePinsBuffersImpl() const
{
	VOXEL_FUNCTION_COUNTER();

	TOptional<bool> Result;
	for (const FVoxelPin& Pin : GetPins())
	{
		if (!EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::TemplatePin))
		{
			continue;
		}

		if (!Result)
		{
			Result = Pin.GetType().IsBuffer();
		}
		else
		{
			ensure(*Result == Pin.GetType().IsBuffer());
		}
	}
	return Result;
}

#if WITH_EDITOR
bool FVoxelNode::IsPinHidden(const FVoxelPin& Pin) const
{
	if (Pin.Metadata.bHidePin)
	{
		return true;
	}

	if (!Pin.Metadata.bShowInDetail)
	{
		return false;
	}

	return !ExposedPins.Contains(Pin.Name);
}

FString FVoxelNode::GetPinDefaultValue(const FVoxelPin& Pin) const
{
	if (!Pin.Metadata.bShowInDetail)
	{
		return Pin.Metadata.DefaultValue;
	}

	const FVoxelNodeExposedPinValue* ExposedPinValue = ExposedPinValues.FindByKey(Pin.Name);
	if (!ensure(ExposedPinValue))
	{
		return Pin.Metadata.DefaultValue;
	}
	return ExposedPinValue->Value.ExportToString();
}

void FVoxelNode::UpdatePropertyBoundDefaultValue(const FVoxelPin& Pin, const FVoxelPinValue& NewValue)
{
	if (!Pin.Metadata.bShowInDetail)
	{
		return;
	}

	if (FVoxelNodeExposedPinValue* PinValue = ExposedPinValues.FindByKey(Pin.Name))
	{
		if (PinValue->Value != NewValue)
		{
			PinValue->Value = NewValue;
			OnExposedPinsUpdated.Broadcast();
		}
	}
	else
	{
		ExposedPinValues.Add({ Pin.Name, NewValue });
		OnExposedPinsUpdated.Broadcast();
	}
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPin& FVoxelNode::GetUniqueInputPin()
{
	FVoxelPin* InputPin = nullptr;
	for (FVoxelPin& Pin : GetPins())
	{
		if (Pin.bIsInput)
		{
			check(!InputPin);
			InputPin = &Pin;
		}
	}
	check(InputPin);

	return *InputPin;
}

FVoxelPin& FVoxelNode::GetUniqueOutputPin()
{
	FVoxelPin* OutputPin = nullptr;
	for (FVoxelPin& Pin : GetPins())
	{
		if (!Pin.bIsInput)
		{
			check(!OutputPin);
			OutputPin = &Pin;
		}
	}
	check(OutputPin);

	return *OutputPin;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FName FVoxelNode::Variadic_AddPin(const FName VariadicPinName, FName PinName)
{
	FlushDeferredPins();

	const TSharedPtr<FVariadicPin> VariadicPin = PrivateNameToVariadicPin.FindRef(VariadicPinName);
	if (!ensure(VariadicPin))
	{
		return {};
	}

	if (PinName.IsNone())
	{
		PinName = VariadicPinName + TEXTVIEW("_0");

		while (
			PrivateNameToPin.Contains(PinName) ||
			PrivateNameToVariadicPin.Contains(PinName))
		{
			PinName.SetNumber(PinName.GetNumber() + 1);
		}
	}

	if (!ensure(!PrivateNameToPin.Contains(PinName)) ||
		!ensure(!PrivateNameToVariadicPin.Contains(PinName)))
	{
		return {};
	}

	FDeferredPin Pin = VariadicPin->PinTemplate;
	Pin.Name = PinName;
	RegisterPin(Pin);

	FixupVariadicPinNames(VariadicPinName);

	return PinName;
}

FName FVoxelNode::Variadic_InsertPin(const FName VariadicPinName, const int32 Position)
{
	FlushDeferredPins();

	const TSharedPtr<FVariadicPin> VariadicPin = PrivateNameToVariadicPin.FindRef(VariadicPinName);
	if (!ensure(VariadicPin))
	{
		return {};
	}

	FName PinName = VariadicPinName + TEXTVIEW("_0");
	while (
		PrivateNameToPin.Contains(PinName) ||
		PrivateNameToVariadicPin.Contains(PinName))
	{
		PinName.SetNumber(PinName.GetNumber() + 1);
	}

	FDeferredPin Pin = VariadicPin->PinTemplate;
	Pin.Name = PinName;
	RegisterPin(Pin);
	SortPins();

	for (int32 Index = Position; Index < VariadicPin->Pins.Num() - 1; Index++)
	{
		VariadicPin->Pins.Swap(Index, VariadicPin->Pins.Num() - 1);
	}

	FixupVariadicPinNames(VariadicPinName);

	return PinName;
}

void FVoxelNode::FixupVariadicPinNames(const FName VariadicPinName)
{
	FlushDeferredPins();

	const TSharedPtr<FVariadicPin> VariadicPin = PrivateNameToVariadicPin.FindRef(VariadicPinName);
	if (!ensure(VariadicPin))
	{
		return;
	}

	for (int32 Index = 0; Index < VariadicPin->Pins.Num(); Index++)
	{
		const FName ArrayPinName = VariadicPin->Pins[Index];
		const TSharedPtr<FVoxelPin> ArrayPin = PrivateNameToPin.FindRef(ArrayPinName);
		if (!ensure(ArrayPin))
		{
			continue;
		}

		ConstCast(ArrayPin->SortOrder) = VariadicPin->PinTemplate.SortOrder + Index / 100.f;

#if WITH_EDITOR
		ConstCast(ArrayPin->Metadata.DisplayName) = FName::NameToDisplayString(VariadicPinName.ToString(), false) + " " + FString::FromInt(Index);
#endif
	}

	SortPins();
}

FName FVoxelNode::CreatePin(
	const FVoxelPinType& Type,
	const bool bIsInput,
	const FName Name,
	const FVoxelPinMetadata& Metadata,
	const EVoxelPinFlags Flags,
	const int32 MinArrayNum)
{
	if (bIsInput)
	{
#if WITH_EDITOR
		ensure(
			Metadata.DefaultValue.IsEmpty() ||
			Type.IsWildcard() ||
			FVoxelPinValue(Type.GetPinDefaultValueType()).ImportFromString(Metadata.DefaultValue));
#endif

		// Use Internal_GetStruct as we might be in a struct constructor
		if (Internal_GetStruct()->IsChildOf(StaticStructFast<FVoxelExecNode>()))
		{
			ensure(Metadata.bVirtualPin);
		}
	}
	else
	{
#if WITH_EDITOR
		ensure(Metadata.DefaultValue.IsEmpty());
#endif

		ensure(!Metadata.bVirtualPin);
		ensure(!Metadata.bConstantPin);
		ensure(!Metadata.bDisplayLast);
		ensure(!Metadata.bNoDefault);
		ensure(!Metadata.bShowInDetail);
	}

	FVoxelPinType BaseType = Type;
	if (Metadata.BaseClass)
	{
		ensure(BaseType.GetInnerType().Is<TSubclassOf<UObject>>());
		BaseType = FVoxelPinType::MakeClass(Metadata.BaseClass);

		if (Type.IsBuffer())
		{
			BaseType = BaseType.GetBufferType();
		}
	}
	if (Metadata.bArrayPin)
	{
		BaseType = BaseType.WithBufferArray(true);
	}

	const FVoxelPinType ChildType = BaseType;

	if (EnumHasAnyFlags(Flags, EVoxelPinFlags::TemplatePin))
	{
		BaseType = FVoxelPinType::MakeWildcard();
	}

	RegisterPin(FDeferredPin
	{
		{},
		MinArrayNum,
		Name,
		bIsInput,
		0,
		Flags,
		BaseType,
		ChildType,
		Metadata
	});

	if (Metadata.bDisplayLast)
	{
		PrivatePinsOrder.Add(Name);
		DisplayLastPins++;
	}
	else
	{
		PrivatePinsOrder.Add(Name);
		if (DisplayLastPins > 0)
		{
			for (int32 Index = PrivatePinsOrder.Num() - DisplayLastPins - 1; Index < PrivatePinsOrder.Num() - 1; Index++)
			{
				PrivatePinsOrder.Swap(Index, PrivatePinsOrder.Num() - 1);
			}
		}
	}

	return Name;
}

void FVoxelNode::RemovePin(const FName Name)
{
	FlushDeferredPins();

	const TSharedPtr<FVoxelPin> Pin = FindPin(Name);
	if (ensure(Pin) &&
		!Pin->VariadicPinName.IsNone() &&
		ensure(PrivateNameToVariadicPin.Contains(Pin->VariadicPinName)))
	{
		ensure(PrivateNameToVariadicPin[Pin->VariadicPinName]->Pins.Remove(Name));
	}

	ensure(PrivateNameToPinBackup.Remove(Name));
	ensure(PrivateNameToPin.Remove(Name));
	PrivatePinsOrder.Remove(Name);

#if WITH_EDITOR
	if (Pin->Metadata.bShowInDetail)
	{
		OnExposedPinsUpdated.Broadcast();
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::FlushDeferredPinsImpl()
{
	ensure(bIsDeferringPins);
	bIsDeferringPins = false;

	for (const FDeferredPin& Pin : DeferredPins)
	{
		RegisterPin(Pin);
	}
	DeferredPins.Empty();
}

void FVoxelNode::RegisterPin(FDeferredPin Pin, const bool bApplyMinNum)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(!Pin.Name.IsNone());
	ensure(Pin.BaseType.IsValid());

	if (Pin.bIsInput &&
		// Don't use IsA as we might be in a constructor
		Internal_GetStruct()->IsChildOf(StaticStructFast<FVoxelExecNode>()))
	{
		// Exec nodes can only have virtual pins
		ensure(Pin.Metadata.bVirtualPin);
	}

	if (Pin.VariadicPinName.IsNone() &&
		Pin.SortOrder == 0)
	{
		Pin.SortOrder = SortOrderCounter++;
	}

	if (Pin.Metadata.bDisplayLast)
	{
		Pin.SortOrder += 10000.f;
	}

	if (bIsDeferringPins)
	{
		DeferredPins.Add(Pin);
		return;
	}

	PrivateNameToPinBackup.Add_EnsureNew(Pin.Name, Pin);

	const FString PinName = Pin.Name.ToString();

#if WITH_EDITOR
	if (!Pin.Metadata.Tooltip.IsSet())
	{
		Pin.Metadata.Tooltip = MakeAttributeLambda([Struct = GetStruct(), Name = Pin.VariadicPinName.IsNone() ? Pin.Name : Pin.VariadicPinName]
		{
			return GVoxelSourceParser->GetPinTooltip(Struct, Name);
		});
	}

	if (Pin.Metadata.DisplayName.IsEmpty())
	{
		Pin.Metadata.DisplayName = FName::NameToDisplayString(PinName, PinName.StartsWith("b", ESearchCase::CaseSensitive));
		Pin.Metadata.DisplayName.RemoveFromStart("Out ");
	}

	if (Pin.Metadata.bShowInDetail)
	{
		OnExposedPinsUpdated.Broadcast();

		if (!ExposedPinValues.FindByKey(Pin.Name))
		{
			FVoxelNodeExposedPinValue ExposedPinValue;
			ExposedPinValue.Name = Pin.Name;
			ExposedPinValue.Value = FVoxelPinValue(Pin.ChildType.GetPinDefaultValueType());

			if (!Pin.Metadata.DefaultValue.IsEmpty())
			{
				ensure(ExposedPinValue.Value.ImportFromString(Pin.Metadata.DefaultValue));
			}

			ExposedPinValues.Add(ExposedPinValue);
		}
	}
#endif

	if (EnumHasAnyFlags(Pin.Flags, EVoxelPinFlags::VariadicPin))
	{
		ensure(Pin.VariadicPinName.IsNone());

		FDeferredPin PinTemplate = Pin;
		PinTemplate.VariadicPinName = Pin.Name;
		EnumRemoveFlags(PinTemplate.Flags, EVoxelPinFlags::VariadicPin);

		PrivateNameToVariadicPin.Add_EnsureNew(Pin.Name, MakeVoxelShared<FVariadicPin>(PinTemplate));

		if (bApplyMinNum)
		{
			for (int32 Index = 0; Index < Pin.MinVariadicNum; Index++)
			{
				Variadic_AddPin(Pin.Name);
			}
		}
	}
	else
	{
		if (!Pin.VariadicPinName.IsNone() &&
			ensure(PrivateNameToVariadicPin.Contains(Pin.VariadicPinName)))
		{
			FVariadicPin& VariadicPin = *PrivateNameToVariadicPin[Pin.VariadicPinName];
#if WITH_EDITOR
			Pin.Metadata.Category = Pin.VariadicPinName.ToString();
#endif
			VariadicPin.Pins.Add(Pin.Name);
		}

		PrivateNameToPin.Add_EnsureNew(Pin.Name, MakeSharedCopy(FVoxelPin(
			Pin.Name,
			Pin.bIsInput,
			Pin.SortOrder,
			Pin.VariadicPinName,
			Pin.Flags,
			Pin.BaseType,
			Pin.ChildType,
			Pin.Metadata)));
	}

	SortPins();
}

void FVoxelNode::SortPins()
{
	PrivateNameToPin.ValueSort([](const TSharedPtr<FVoxelPin>& A, const TSharedPtr<FVoxelPin>& B)
	{
		if (A->bIsInput != B->bIsInput)
		{
			return A->bIsInput > B->bIsInput;
		}

		return A->SortOrder < B->SortOrder;
	});
}

void FVoxelNode::SortVariadicPinNames(const FName VariadicPinName)
{
	VOXEL_FUNCTION_COUNTER();

	const TSharedPtr<FVariadicPin> VariadicPin = PrivateNameToVariadicPin.FindRef(VariadicPinName);
	if (!VariadicPin)
	{
		return;
	}

	TArray<int32> SortOrders;
	TArray<TSharedPtr<FVoxelPin>> AffectedPins;
	for (const FName PinName : VariadicPin->Pins)
	{
		if (const TSharedPtr<FVoxelPin> Pin = PrivateNameToPin.FindRef(PinName))
		{
			SortOrders.Add(Pin->SortOrder);
		}
	}

	SortOrders.Sort();
	for (int32 Index = 0; Index < AffectedPins.Num(); Index++)
	{
		ConstCast(AffectedPins[Index]->SortOrder) = SortOrders[Index];
	}

	SortPins();
}

FVoxelNodeSerializedData FVoxelNode::GetSerializedData() const
{
	FlushDeferredPins();

	FVoxelNodeSerializedData SerializedData;
	SerializedData.bIsValid = true;

	for (const auto& It : PrivateNameToPin)
	{
		const FVoxelPin& Pin = *It.Value;
		if (!Pin.IsPromotable())
		{
			continue;
		}

		SerializedData.NameToPinType.Add(Pin.Name, Pin.GetType());
	}

	for (const auto& It : PrivateNameToVariadicPin)
	{
		FVoxelNodeVariadicPinSerializedData VariadicPinSerializedData;
		VariadicPinSerializedData.PinNames = It.Value->Pins;
		SerializedData.VariadicPinNameToSerializedData.Add(It.Key, VariadicPinSerializedData);
	}

#if WITH_EDITOR
	SerializedData.ExposedPins = ExposedPins;
	SerializedData.ExposedPinsValues = ExposedPinValues;
#endif

	return SerializedData;
}

void FVoxelNode::LoadSerializedData(const FVoxelNodeSerializedData& SerializedData)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(SerializedData.bIsValid);

#if WITH_EDITOR
	ExposedPins = SerializedData.ExposedPins;
	ExposedPinValues = SerializedData.ExposedPinsValues;
#endif

	FlushDeferredPins();

	for (const auto& It : PrivateNameToVariadicPin)
	{
		FVariadicPin& VariadicPin = *It.Value;

		const TVoxelArray<FName> Pins = VariadicPin.Pins;
		for (const FName Name : Pins)
		{
			RemovePin(Name);
		}
		ensure(VariadicPin.Pins.Num() == 0);
	}

	for (const auto& It : SerializedData.VariadicPinNameToSerializedData)
	{
		const TSharedPtr<FVariadicPin> VariadicPin = PrivateNameToVariadicPin.FindRef(It.Key);
		if (!ensureVoxelSlow(VariadicPin))
		{
			continue;
		}

		for (const FName Name : It.Value.PinNames)
		{
			Variadic_AddPin(It.Key, Name);
		}
	}

	// Ensure MinNum is applied
	for (const auto& It : PrivateNameToVariadicPin)
	{
		FVariadicPin& VariadicPin = *It.Value;

		for (int32 Index = VariadicPin.Pins.Num(); Index < VariadicPin.PinTemplate.MinVariadicNum; Index++)
		{
			Variadic_AddPin(It.Key);
		}
	}

	for (const auto& It : SerializedData.NameToPinType)
	{
		const TSharedPtr<FVoxelPin> Pin = PrivateNameToPin.FindRef(It.Key);
		if (!Pin ||
			!Pin->IsPromotable())
		{
			continue;
		}

		if (!It.Value.IsValid())
		{
			// Old type that was removed
			continue;
		}

#if WITH_EDITOR
		if (!It.Value.IsWildcard() &&
			!ensureVoxelSlow(GetPromotionTypes(*Pin).Contains(It.Value)))
		{
			continue;
		}
#endif

		Pin->SetType(It.Value);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelNode::InitializeNodeRuntime(
	const FVoxelGraphNodeRef& NodeRef,
	const bool bIsCallNode)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(NodeRef.TerminalGraphRef.Graph.IsValid() || bIsCallNode);
	ensure(!NodeRef.NodeId.IsNone() || bIsCallNode);
	ensure(!NodeRef.IsDeleted());

	FlushDeferredPins();

	FString DebugName = GetStruct()->GetName();
	DebugName.RemoveFromStart("VoxelNode_");
	DebugName.RemoveFromStart("VoxelMathNode_");
	DebugName.RemoveFromStart("VoxelTemplateNode_");

	check(!NodeRuntime);
	NodeRuntime = MakeVoxelShared<FVoxelNodeRuntime>();
	NodeRuntime->Node = this;
	NodeRuntime->NodeRef = NodeRef;
	NodeRuntime->AreTemplatePinsBuffers = AreTemplatePinsBuffersImpl();
	NodeRuntime->bIsCallNode = bIsCallNode;

	for (const auto& It : PrivateNameToVariadicPin)
	{
		NodeRuntime->VariadicPinNameToPinNames.Add_CheckNew(It.Key, It.Value->Pins);
	}

	// Make sure pin datas are available when pre-compiling/compiling
	for (const FVoxelPin& Pin : GetPins())
	{
		const TSharedRef<FVoxelNodeRuntime::FPinData> PinData = MakeVoxelShared<FVoxelNodeRuntime::FPinData>(
			Pin.GetType(),
			Pin.bIsInput,
			FName(DebugName + "." + Pin.Name.ToString()),
			FVoxelGraphPinRef{ NodeRef, Pin.Name },
			Pin.Metadata);

		NodeRuntime->NameToPinData.Add_CheckNew(Pin.Name, PinData);
	}

	PreCompile();

	for (const FVoxelPin& Pin : GetPins())
	{
		FVoxelNodeRuntime::FPinData& PinData = *NodeRuntime->NameToPinData[Pin.Name];

		if (Pin.bIsInput)
		{
			if (Pin.Metadata.bVirtualPin)
			{
				const FVoxelGraphPinRef PinRef
				{
					NodeRef,
					Pin.Name
				};
				const TSharedRef<FVoxelGraphEvaluatorRef> EvaluatorRef = GVoxelGraphEvaluatorManager->MakeEvaluatorRef_GameThread(PinRef);

				PinData.Compute = MakeVoxelShared<FVoxelComputeValue>([
					Type = Pin.GetType(),
					EvaluatorRef](const FVoxelQuery& Query)
				{
					// Wrap in a task to check type
					return
						MakeVoxelTask()
						.Execute(Type, [=]
						{
							return EvaluatorRef->Compute(Query);
						});
				});
			}
			else
			{
				// Assigned by compiler utilities
				// Exec nodes can only have virtual pins
				ensure(!IsA<FVoxelExecNode>());
			}
		}
		else
		{
			FVoxelComputeValue Compute = CompileCompute(Pin.Name);
			if (!ensure(Compute))
			{
				VOXEL_MESSAGE(Error, "INTERNAL ERROR: {0}.{1} has no Compute", this, Pin.Name);
				return;
			}

			PinData.Compute = MakeSharedCopy(MoveTemp(Compute));
		}
	}
}

void FVoxelNode::RemoveEditorData()
{
	VOXEL_FUNCTION_COUNTER();

	ensure(!bEditorDataRemoved);
	bEditorDataRemoved = true;

	DeferredPins.Empty();
	PrivateNameToPinBackup.Empty();
	PrivatePinsOrder.Empty();
	PrivateNameToPin.Empty();
	PrivateNameToVariadicPin.Empty();

#if WITH_EDITOR
	ExposedPinValues.Empty();
	ExposedPins.Empty();
#endif

	SerializedDataProperty = {};

	UpdateStats();
}

void FVoxelNode::EnableSharedNode(const TSharedRef<FVoxelNode>& SharedThis)
{
	check(this == &SharedThis.Get());
	check(!WeakThis.IsValid());
	WeakThis = SharedThis;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition::GetInputs() const
{
	return GetPins(true);
}

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition::GetOutputs() const
{
	return GetPins(false);
}

TSharedPtr<const IVoxelNodeDefinition::FNode> FVoxelNodeDefinition::GetPins(const bool bIsInput) const
{
	const TSharedRef<FCategoryNode> RootNode = FNode::MakeRoot(bIsInput);

	for (const FName PinName : Node.PrivatePinsOrder)
	{
		const FVoxelNode::FDeferredPin* Pin = Node.PrivateNameToPinBackup.Find(PinName);
		if (!ensure(Pin) ||
			Pin->bIsInput != bIsInput ||
			Pin->IsVariadicChild())
		{
			continue;
		}

		if (Pin->IsVariadicRoot())
		{
			const TSharedRef<FVariadicPinNode> VariadicPinNode = RootNode->FindOrAddCategory(Pin->Metadata.Category)->AddVariadicPin(PinName);
			const TSharedPtr<FVoxelNode::FVariadicPin> VariadicPin = Node.PrivateNameToVariadicPin.FindRef(PinName);
			if (!ensure(VariadicPin))
			{
				continue;
			}

			for (const FName ArrayPin : VariadicPin->Pins)
			{
				VariadicPinNode->AddPin(ArrayPin);
			}
			continue;
		}

		RootNode->FindOrAddCategory(Pin->Metadata.Category)->AddPin(PinName);
	}

	return RootNode;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
FString FVoxelNodeDefinition::GetAddPinLabel() const
{
	return "Add pin";
}

FString FVoxelNodeDefinition::GetAddPinTooltip() const
{
	return "Add pin";
}

FString FVoxelNodeDefinition::GetRemovePinTooltip() const
{
	return "Remove pin";
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
bool FVoxelNodeDefinition::Variadic_CanAddPinTo(const FName VariadicPinName) const
{
	ensure(!VariadicPinName.IsNone());
	return Node.PrivateNameToVariadicPin.Contains(VariadicPinName);
}

FName FVoxelNodeDefinition::Variadic_AddPinTo(const FName VariadicPinName)
{
	ensure(!VariadicPinName.IsNone());
	return Node.Variadic_AddPin(VariadicPinName);
}

bool FVoxelNodeDefinition::Variadic_CanRemovePinFrom(const FName VariadicPinName) const
{
	ensure(!VariadicPinName.IsNone());

	const TSharedPtr<FVoxelNode::FVariadicPin> VariadicPin = Node.PrivateNameToVariadicPin.FindRef(VariadicPinName);

	return
		VariadicPin &&
		VariadicPin->Pins.Num() > VariadicPin->PinTemplate.MinVariadicNum;
}

void FVoxelNodeDefinition::Variadic_RemovePinFrom(const FName VariadicPinName)
{
	ensure(!VariadicPinName.IsNone());

	const TSharedPtr<FVoxelNode::FVariadicPin> VariadicPin = Node.PrivateNameToVariadicPin.FindRef(VariadicPinName);
	if (!ensure(VariadicPin) ||
		!ensure(VariadicPin->Pins.Num() > 0) ||
		!ensure(VariadicPin->Pins.Num() > VariadicPin->PinTemplate.MinVariadicNum))
	{
		return;
	}

	const FName PinName = VariadicPin->Pins.Last();
	Node.RemovePin(PinName);

	Node.ExposedPins.Remove(PinName);
	const int32 ExposedPinIndex = Node.ExposedPinValues.IndexOfByKey(PinName);
	if (ExposedPinIndex != -1)
	{
		Node.ExposedPinValues.RemoveAt(ExposedPinIndex);
	}

	Node.FixupVariadicPinNames(VariadicPinName);
}

bool FVoxelNodeDefinition::CanRemoveSelectedPin(const FName PinName) const
{
	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);
	if (!Pin)
	{
		return false;
	}

	if (Pin->VariadicPinName.IsNone())
	{
		return false;
	}

	return Variadic_CanRemovePinFrom(Pin->VariadicPinName);
}

void FVoxelNodeDefinition::RemoveSelectedPin(const FName PinName)
{
	if (!ensure(CanRemoveSelectedPin(PinName)))
	{
		return;
	}

	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);

	if (Pin->Metadata.bShowInDetail)
	{
		Node.ExposedPins.Remove(Pin->Name);
		const int32 ExposedPinIndex = Node.ExposedPinValues.IndexOfByKey(Pin->Name);
		if (ExposedPinIndex != -1)
		{
			Node.ExposedPinValues.RemoveAt(ExposedPinIndex);
		}
	}

	Node.RemovePin(Pin->Name);

	Node.FixupVariadicPinNames(Pin->VariadicPinName);
}

void FVoxelNodeDefinition::InsertPinBefore(const FName PinName)
{
	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);
	if (!Pin ||
		Pin->VariadicPinName.IsNone())
	{
		return;
	}

	const TSharedPtr<FVoxelNode::FVariadicPin> VariadicPin = Node.PrivateNameToVariadicPin.FindRef(Pin->VariadicPinName);
	if (!VariadicPin)
	{
		return;
	}

	const int32 PinPosition = VariadicPin->Pins.IndexOfByPredicate([&Pin] (const FName& Name)
	{
		return Pin->Name == Name;
	});

	if (!ensure(PinPosition != -1))
	{
		return;
	}

	const FName NewPinName = Node.Variadic_InsertPin(Pin->VariadicPinName, PinPosition);
	Node.SortVariadicPinNames(Pin->VariadicPinName);

	if (Pin->Metadata.bShowInDetail)
	{
		if (Node.ExposedPins.Contains(Pin->Name))
		{
			Node.ExposedPins.Add(NewPinName);
		}
	}
}

void FVoxelNodeDefinition::DuplicatePin(const FName PinName)
{
	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);
	if (!Pin ||
		Pin->VariadicPinName.IsNone())
	{
		return;
	}

	const TSharedPtr<FVoxelNode::FVariadicPin> PinArray = Node.PrivateNameToVariadicPin.FindRef(Pin->VariadicPinName);
	if (!PinArray)
	{
		return;
	}

	const int32 PinPosition = PinArray->Pins.IndexOfByPredicate([&Pin] (const FName& Name)
	{
		return Pin->Name == Name;
	});

	if (!ensure(PinPosition != -1))
	{
		return;
	}

	const FName NewPinName = Node.Variadic_InsertPin(Pin->VariadicPinName, PinPosition + 1);
	Node.SortVariadicPinNames(Pin->VariadicPinName);

	if (Pin->Metadata.bShowInDetail)
	{
		FVoxelPinValue NewValue;
		if (const FVoxelNodeExposedPinValue* PinValue = Node.ExposedPinValues.FindByKey(Pin->Name))
		{
			NewValue = PinValue->Value;
		}

		Node.ExposedPinValues.Add({ NewPinName, NewValue });

		if (Node.ExposedPins.Contains(Pin->Name))
		{
			Node.ExposedPins.Add(NewPinName);
		}
	}
}

void FVoxelNodeDefinition::ExposePin(const FName PinName)
{
	const TSharedPtr<FVoxelPin> Pin = Node.FindPin(PinName);
	if (!Pin ||
		!Pin->Metadata.bShowInDetail)
	{
		return;
	}

	Node.ExposedPins.Add(PinName);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelComputeValue& FVoxelNodeCaller::FBindings::Bind(const FName Name) const
{
	FVoxelNodeRuntime::FPinData& PinData = *NodeRuntime.NameToPinData[Name];
	ensure(!PinData.Compute);

	TVoxelUniquePtr<FVoxelComputeValue> NewCompute = MakeVoxelUnique<FVoxelComputeValue>();
	FVoxelComputeValue* NewComputePtr = NewCompute.Get();

	PinData.Compute = MakeVoxelShared<FVoxelComputeValue>(
		[NewCompute = MoveTemp(NewCompute), Type = PinData.Type](const FVoxelQuery& Query) -> FVoxelFutureValue
		{
			const FVoxelFutureValue Value = (*NewCompute)(Query);
			if (!Value.IsValid())
			{
				return FVoxelRuntimePinValue(Type);
			}
			return Value;
		});

	return *NewComputePtr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelCriticalSection GVoxelNodePoolsCriticalSection;
TArray<FVoxelNodeCaller::FNodePool*> GVoxelNodePools_RequiresLock;

void PurgeVoxelNodePools()
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_LOCK(GVoxelNodePoolsCriticalSection);

	for (FVoxelNodeCaller::FNodePool* Pool : GVoxelNodePools_RequiresLock)
	{
		VOXEL_SCOPE_LOCK(Pool->CriticalSection);
		Pool->Nodes.Empty();
	}
}

VOXEL_CONSOLE_COMMAND(
	PurgeNodePools,
	"voxel.PurgeNodePools",
	"")
{
	PurgeVoxelNodePools();
}

VOXEL_RUN_ON_STARTUP_GAME(RegisterPurgeVoxelNodePools)
{
	GOnVoxelModuleUnloaded_DoCleanup.AddLambda([]
	{
		PurgeVoxelNodePools();
	});
}

FVoxelNodeCaller::FNodePool::FNodePool()
{
	VOXEL_SCOPE_LOCK(GVoxelNodePoolsCriticalSection);
	GVoxelNodePools_RequiresLock.Add(this);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelFutureValue FVoxelNodeCaller::CallNode(
	FNodePool& Pool,
	const UScriptStruct* Struct,
	const FName StatName,
	const FVoxelQuery& Query,
	const FVoxelPinRef OutputPin,
	const TFunctionRef<void(FBindings&, FVoxelNode& Node)> Bind)
{
	VOXEL_SCOPE_COUNTER_FNAME(StatName);

	TSharedPtr<FVoxelNode> Node;
	{
		VOXEL_SCOPE_LOCK(Pool.CriticalSection);

		if (Pool.Nodes.Num() > 0)
		{
			Node = Pool.Nodes.Pop();
		}
	}

	if (!Node)
	{
		VOXEL_SCOPE_COUNTER("Allocate new node");
		VOXEL_ALLOW_MALLOC_SCOPE();
		VOXEL_ALLOW_REALLOC_SCOPE();

		Node = MakeSharedStruct<FVoxelNode>(Struct);

		for (FVoxelPin& Pin : Node->GetPins())
		{
			// Calling nodes taking variadic pins is not supported
			ensure(Pin.VariadicPinName.IsNone());
			// Virtual pins are not supported
			ensure(!Pin.Metadata.bVirtualPin);
		}

		// Promote all template pins to buffers
		for (FVoxelPin& Pin : Node->GetPins())
		{
			if (EnumHasAllFlags(Pin.Flags, EVoxelPinFlags::TemplatePin))
			{
				Pin.SetType(Pin.GetType().GetBufferType());
			}
		}

		Node->InitializeNodeRuntime({}, true);
		Node->RemoveEditorData();
	}

	FVoxelNodeRuntime& NodeRuntime = ConstCast(Node->GetNodeRuntime());
	NodeRuntime.NodeRef = Query.GetTopNode();

	FBindings Bindings(NodeRuntime);
	Bind(Bindings, *Node);

	for (auto& It : NodeRuntime.NameToPinData)
	{
		FVoxelNodeRuntime::FPinData& PinData = *It.Value;
		if (!PinData.bIsInput)
		{
			continue;
		}

		if (ensure(PinData.Compute))
		{
			continue;
		}

		PinData.Compute = MakeVoxelShared<FVoxelComputeValue>([Type = PinData.Type](const FVoxelQuery&)
		{
			return FVoxelFutureValue(FVoxelRuntimePinValue(Type));
		});
	}

	const FVoxelNodeRuntime::FPinData& PinData = NodeRuntime.GetPinData(OutputPin);
	check(!PinData.bIsInput);

	const FVoxelFutureValue Result = (*PinData.Compute)(Query.EnterScope(*Node));

	// Use a destructor so that the node is returned to pool even if the task is cancelled
	struct FReturnToPool
	{
		FNodePool& Pool;
		TSharedRef<FVoxelNode> Node;

		~FReturnToPool()
		{
			Node->ReturnToPool();

			VOXEL_SCOPE_LOCK(Pool.CriticalSection);
			VOXEL_ALLOW_REALLOC_SCOPE();
			Pool.Nodes.Add(Node);
		}
	};
	const TSharedRef<FReturnToPool> ReturnToPool = MakeVoxelShareable(new (GVoxelMemory) FReturnToPool
	{
		Pool,
		Node.ToSharedRef()
	});

	return
		MakeVoxelTask("Return node to pool")
		.Dependency(Result)
		.Execute(PinData.Type, [Result, ReturnToPool]
		{
			(void)ReturnToPool;
			return Result;
		});
}