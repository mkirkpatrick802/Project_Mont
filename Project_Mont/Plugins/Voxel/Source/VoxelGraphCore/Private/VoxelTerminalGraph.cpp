// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelTerminalGraph.h"
#include "VoxelGraph.h"
#include "VoxelExecNode.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraphRuntime.h"

void FVoxelGraphInput::Fixup()
{
	DefaultValue.Fixup(Type.GetExposedType());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelGraph& UVoxelTerminalGraph::GetGraph()
{
	return *GetOuterUVoxelGraph();
}

const UVoxelGraph& UVoxelTerminalGraph::GetGraph() const
{
	return *GetOuterUVoxelGraph();
}

FGuid UVoxelTerminalGraph::GetGuid() const
{
	if (GetGraph().FindTerminalGraph_NoInheritance(PrivateGuid) != this)
	{
		ensure(!PrivateGuid.IsValid());
		ensure(!GetGraph().FindTerminalGraph_NoInheritance(PrivateGuid));

		ConstCast(this)->PrivateGuid = GetGraph().FindTerminalGraphGuid_NoInheritance(this);
		ensure(PrivateGuid.IsValid());
	}
	return PrivateGuid;
}

void UVoxelTerminalGraph::SetGuid_Hack(const FGuid& Guid)
{
	PrivateGuid = Guid;
}

#if WITH_EDITOR
FString UVoxelTerminalGraph::GetDisplayName() const
{
	return GetMetadata().DisplayName;
}

FVoxelGraphMetadata UVoxelTerminalGraph::GetMetadata() const
{
	if (IsMainTerminalGraph())
	{
		return GetGraph().GetMetadata();
	}

	const UVoxelTerminalGraph* TopmostTerminalGraph = GetGraph().FindTopmostTerminalGraph(GetGuid());
	if (!ensure(TopmostTerminalGraph))
	{
		return {};
	}

	return TopmostTerminalGraph->PrivateMetadata;
}

void UVoxelTerminalGraph::UpdateMetadata(TFunctionRef<void(FVoxelGraphMetadata&)> Lambda)
{
	if (!ensure(!IsMainTerminalGraph()) ||
		!ensure(IsTopmostTerminalGraph()))
	{
		return;
	}

	Lambda(PrivateMetadata);

	if (PrivateMetadata.DisplayName.IsEmpty())
	{
		PrivateMetadata.DisplayName = "MyFunction";
	}

	GVoxelGraphTracker->NotifyTerminalGraphMetaDataChanged(*this);
}
#endif

#if WITH_EDITOR
void UVoxelTerminalGraph::SetEdGraph_Hack(UEdGraph* NewEdGraph)
{
	EdGraph = NewEdGraph;
}

void UVoxelTerminalGraph::SetDisplayName_Hack(const FString& Name)
{
	PrivateMetadata.DisplayName = Name;
}

UEdGraph& UVoxelTerminalGraph::GetEdGraph()
{
	if (!EdGraph)
	{
		check(GVoxelGraphEditorInterface);
		EdGraph = GVoxelGraphEditorInterface->CreateEdGraph(*this);
	}
	return *EdGraph;
}

const UEdGraph& UVoxelTerminalGraph::GetEdGraph() const
{
	return ConstCast(this)->GetEdGraph();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FVoxelGraphInput* UVoxelTerminalGraph::FindInput(const FGuid& Guid) const
{
	for (const UVoxelTerminalGraph* TerminalGraph : GetBaseTerminalGraphs())
	{
		if (const FVoxelGraphInput* Input = TerminalGraph->GuidToInput.Find(Guid))
		{
			return Input;
		}
	}
	return nullptr;
}

const FVoxelGraphOutput* UVoxelTerminalGraph::FindOutput(const FGuid& Guid) const
{
	for (const UVoxelTerminalGraph* TerminalGraph : GetBaseTerminalGraphs())
	{
		if (const FVoxelGraphOutput* Output = TerminalGraph->GuidToOutput.Find(Guid))
		{
			return Output;
		}
	}
	return nullptr;
}

const FVoxelGraphOutput* UVoxelTerminalGraph::FindOutputByName(const FName& Name, FGuid& OutGuid) const
{
	for (const UVoxelTerminalGraph* TerminalGraph : GetBaseTerminalGraphs())
	{
		for (const auto& It : TerminalGraph->GuidToOutput)
		{
			if (It.Value.Name == Name)
			{
				OutGuid = It.Key;
				return &It.Value;
			}
		}
	}
	return nullptr;
}

const FVoxelGraphLocalVariable* UVoxelTerminalGraph::FindLocalVariable(const FGuid& Guid) const
{
	// No inheritance
	return GuidToLocalVariable.Find(Guid);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FVoxelGraphInput& UVoxelTerminalGraph::FindInputChecked(const FGuid& Guid) const
{
	const FVoxelGraphInput* Input = FindInput(Guid);
	check(Input);
	return *Input;
}

const FVoxelGraphOutput& UVoxelTerminalGraph::FindOutputChecked(const FGuid& Guid) const
{
	const FVoxelGraphOutput* Output = FindOutput(Guid);
	check(Output);
	return *Output;
}

const FVoxelGraphLocalVariable& UVoxelTerminalGraph::FindLocalVariableChecked(const FGuid& Guid) const
{
	const FVoxelGraphLocalVariable* LocalVariable = FindLocalVariable(Guid);
	check(LocalVariable);
	return *LocalVariable;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelSet<FGuid> UVoxelTerminalGraph::GetInputs() const
{
	TVoxelSet<FGuid> Result;
	// Add parent inputs first
	for (const UVoxelTerminalGraph* TerminalGraph : GetBaseTerminalGraphs().Reverse())
	{
		for (const auto& It : TerminalGraph->GuidToInput)
		{
			ensure(!Result.Contains(It.Key));
			Result.Add(It.Key);
		}
	}
	return Result;
}

TVoxelSet<FGuid> UVoxelTerminalGraph::GetOutputs() const
{
	TVoxelSet<FGuid> Result;
	// Add parent outputs first
	for (const UVoxelTerminalGraph* TerminalGraph : GetBaseTerminalGraphs().Reverse())
	{
		for (const auto& It : TerminalGraph->GuidToOutput)
		{
			ensure(!Result.Contains(It.Key));
			Result.Add(It.Key);
		}
	}
	return Result;
}

TVoxelSet<FGuid> UVoxelTerminalGraph::GetLocalVariables() const
{
	TVoxelSet<FGuid> Result;
	for (const auto& It : GuidToLocalVariable)
	{
		Result.Add(It.Key);
	}
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelTerminalGraph::IsInheritedInput(const FGuid& Guid) const
{
	return
		ensure(FindInput(Guid)) &&
		!GuidToInput.Contains(Guid);
}

bool UVoxelTerminalGraph::IsInheritedOutput(const FGuid& Guid) const
{
	return
		ensure(FindOutput(Guid)) &&
		!GuidToOutput.Contains(Guid);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelTerminalGraph::AddInput(const FGuid& Guid, const FVoxelGraphInput& Input)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(!FindInput(Guid));
	GuidToInput.Add(Guid, Input);

	GVoxelGraphTracker->NotifyInputChanged(*this);
}

void UVoxelTerminalGraph::AddOutput(const FGuid& Guid, const FVoxelGraphOutput& Output)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(!FindOutput(Guid));
	GuidToOutput.Add(Guid, Output);

	GVoxelGraphTracker->NotifyOutputChanged(*this);
}

void UVoxelTerminalGraph::AddLocalVariable(const FGuid& Guid, const FVoxelGraphLocalVariable& LocalVariable)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(!FindLocalVariable(Guid));
	GuidToLocalVariable.Add(Guid, LocalVariable);

	GVoxelGraphTracker->NotifyLocalVariableChanged(*this);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelTerminalGraph::RemoveInput(const FGuid& Guid)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(GuidToInput.Remove(Guid));

	GVoxelGraphTracker->NotifyInputChanged(*this);
}

void UVoxelTerminalGraph::RemoveOutput(const FGuid& Guid)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(GuidToOutput.Remove(Guid));

	GVoxelGraphTracker->NotifyOutputChanged(*this);
}

void UVoxelTerminalGraph::RemoveLocalVariable(const FGuid& Guid)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(GuidToLocalVariable.Remove(Guid));

	GVoxelGraphTracker->NotifyLocalVariableChanged(*this);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelTerminalGraph::UpdateInput(const FGuid& Guid, TFunctionRef<void(FVoxelGraphInput&)> Update)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelGraphInput* Input = GuidToInput.Find(Guid);
	if (!ensure(Input))
	{
		return;
	}
	const FVoxelGraphInput PreviousInput = *Input;

	Update(*Input);

	// If category changed move the member to the end to avoid reordering an entire category as a side effect
	if (PreviousInput.Category != Input->Category)
	{
		FVoxelGraphInput InputCopy;
		verify(GuidToInput.RemoveAndCopyValue(Guid, InputCopy));
		GuidToInput.CompactStable();
		GuidToInput.Add(Guid, InputCopy);
	}

	GVoxelGraphTracker->NotifyInputChanged(*this);
}

void UVoxelTerminalGraph::UpdateOutput(const FGuid& Guid, TFunctionRef<void(FVoxelGraphOutput&)> Update)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelGraphOutput* Output = GuidToOutput.Find(Guid);
	if (!ensure(Output))
	{
		return;
	}
	const FVoxelGraphOutput PreviousOutput = *Output;

	Update(*Output);

	// If category changed move the member to the end to avoid reordering an entire category as a side effect
	if (PreviousOutput.Category != Output->Category)
	{
		FVoxelGraphOutput OutputCopy;
		verify(GuidToOutput.RemoveAndCopyValue(Guid, OutputCopy));
		GuidToOutput.CompactStable();
		GuidToOutput.Add(Guid, OutputCopy);
	}

	GVoxelGraphTracker->NotifyOutputChanged(*this);
}

void UVoxelTerminalGraph::UpdateLocalVariable(const FGuid& Guid, TFunctionRef<void(FVoxelGraphLocalVariable&)> Update)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelGraphLocalVariable* LocalVariable = GuidToLocalVariable.Find(Guid);
	if (!ensure(LocalVariable))
	{
		return;
	}
	const FVoxelGraphLocalVariable PreviousLocalVariable = *LocalVariable;

	Update(*LocalVariable);

	// If category changed move the member to the end to avoid reordering an entire category as a side effect
	if (PreviousLocalVariable.Category != LocalVariable->Category)
	{
		FVoxelGraphLocalVariable LocalVariableCopy;
		verify(GuidToLocalVariable.RemoveAndCopyValue(Guid, LocalVariableCopy));
		GuidToLocalVariable.CompactStable();
		GuidToLocalVariable.Add(Guid, LocalVariableCopy);
	}

	GVoxelGraphTracker->NotifyLocalVariableChanged(*this);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelTerminalGraph::ReorderInputs(const TConstVoxelArrayView<FGuid> NewGuids)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FGuid> FinalGuids;
	for (const FGuid& Guid : NewGuids)
	{
		ensure(FindInput(Guid));

		if (GuidToInput.Contains(Guid))
		{
			FinalGuids.Add(Guid);
		}
	}
	FVoxelUtilities::ReorderMapKeys(GuidToInput, FinalGuids);

	GVoxelGraphTracker->NotifyInputChanged(*this);
}

void UVoxelTerminalGraph::ReorderOutputs(const TConstVoxelArrayView<FGuid> NewGuids)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FGuid> FinalGuids;
	for (const FGuid& Guid : NewGuids)
	{
		ensure(FindOutput(Guid));

		if (GuidToOutput.Contains(Guid))
		{
			FinalGuids.Add(Guid);
		}
	}
	FVoxelUtilities::ReorderMapKeys(GuidToOutput, FinalGuids);

	GVoxelGraphTracker->NotifyOutputChanged(*this);
}

auto UVoxelTerminalGraph::ReorderLocalVariables(const TConstVoxelArrayView<FGuid> NewGuids) -> void
{
	VOXEL_FUNCTION_COUNTER();

	// No fixup needed because no inheritance
	FVoxelUtilities::ReorderMapKeys(GuidToLocalVariable, NewGuids);

	GVoxelGraphTracker->NotifyLocalVariableChanged(*this);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelTerminalGraph::UVoxelTerminalGraph()
{
	Runtime = CreateDefaultSubobject<UVoxelTerminalGraphRuntime>("Runtime");

	SetFlags(RF_Public);
	Runtime->SetFlags(RF_Public);
}

void UVoxelTerminalGraph::Fixup()
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!HasAnyFlags(RF_NeedPostLoad));

	// Fixup GUID
	{
		const FGuid Guid = GetGraph().FindTerminalGraphGuid_NoInheritance(this);
		ensure(!PrivateGuid.IsValid() || PrivateGuid == Guid);

		PrivateGuid = Guid;
	}

	// Fixup display name
#if WITH_EDITOR
	if (!IsMainTerminalGraph() &&
		IsTopmostTerminalGraph() &&
		PrivateMetadata.DisplayName.IsEmpty())
	{
		PrivateMetadata.DisplayName = "MyFunction";
	}
#endif

	// Fixup input default values
	for (auto& It : GuidToInput)
	{
		It.Value.Fixup();
	}

	const auto FixupMap = [&](auto& GuidToProperty, TSet<FName> Names = {})
	{
		// Ensure names are unique
		for (auto& It : GuidToProperty)
		{
			FVoxelGraphProperty& Property = It.Value;
#if WITH_EDITOR
			Property.Category = FVoxelUtilities::SanitizeCategory(Property.Category);
#endif

			if (Property.Name.IsNone())
			{
				Property.Name = "MyMember";
			}

			while (Names.Contains(Property.Name))
			{
				Property.Name.SetNumber(Property.Name.GetNumber() + 1);
			}

			Names.Add(Property.Name);
		}

		using Type = typename VOXEL_GET_TYPE(GuidToProperty)::ValueType;

#if WITH_EDITOR
		// Sort elements by category
		TMap<FString, TMap<FGuid, Type>> CategoryToGuidToProperty;
		for (const auto& It : GuidToProperty)
		{
			CategoryToGuidToProperty.FindOrAdd(It.Value.Category).Add(It.Key, It.Value);
		}

		GuidToProperty.Empty();
		for (const auto& CategoryIt : CategoryToGuidToProperty)
		{
			for (const auto& It : CategoryIt.Value)
			{
				GuidToProperty.Add(It.Key, It.Value);
			}
		}
#endif
	};

	TSet<FName> GraphInputsNames;
	if (!IsMainTerminalGraph())
	{
		const UVoxelTerminalGraph& MainTerminalGraph = GetGraph().GetMainTerminalGraph();
		for (const FGuid& Guid : MainTerminalGraph.GetInputs())
		{
			const FVoxelGraphInput& Input = MainTerminalGraph.FindInputChecked(Guid);
			GraphInputsNames.Add(Input.Name);
		}
	}

	FixupMap(GuidToInput, GraphInputsNames);
	FixupMap(GuidToOutput);
	FixupMap(GuidToLocalVariable);
}

bool UVoxelTerminalGraph::HasExecNodes() const
{
	for (const UVoxelTerminalGraph* BaseTerminalGraph : GetBaseTerminalGraphs())
	{
		if (BaseTerminalGraph->GetRuntime().HasNode<FVoxelExecNode>())
		{
			return true;
		}
	}
	return false;
}

bool UVoxelTerminalGraph::IsMainTerminalGraph() const
{
	return GetGuid() == GVoxelMainTerminalGraphGuid;
}

bool UVoxelTerminalGraph::IsTopmostTerminalGraph() const
{
	ensure(!IsMainTerminalGraph());
	return GetGraph().FindTopmostTerminalGraph(GetGuid()) == this;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelInlineSet<UVoxelTerminalGraph*, 8> UVoxelTerminalGraph::GetBaseTerminalGraphs()
{
	const FGuid Guid = GetGuid();

	TVoxelInlineSet<UVoxelTerminalGraph*, 8> Result;
	for (UVoxelGraph* BaseGraph : GetGraph().GetBaseGraphs())
	{
		UVoxelTerminalGraph* TerminalGraph = BaseGraph->FindTerminalGraph_NoInheritance(Guid);
		if (!TerminalGraph ||
			!ensure(!Result.Contains(TerminalGraph)))
		{
			continue;
		}

		Result.Add_CheckNew(TerminalGraph);
	}
	return Result;
}

TVoxelInlineSet<const UVoxelTerminalGraph*, 8> UVoxelTerminalGraph::GetBaseTerminalGraphs() const
{
	return ReinterpretCastRef<TVoxelInlineSet<const UVoxelTerminalGraph*, 8>>(ConstCast(this)->GetBaseTerminalGraphs());
}

TVoxelSet<UVoxelTerminalGraph*> UVoxelTerminalGraph::GetChildTerminalGraphs_LoadedOnly()
{
	VOXEL_FUNCTION_COUNTER();

	const FGuid Guid = GetGuid();

	TVoxelSet<UVoxelTerminalGraph*> Result;
	for (UVoxelGraph* ChildGraph : GetGraph().GetChildGraphs_LoadedOnly())
	{
		UVoxelTerminalGraph* TerminalGraph = ChildGraph->FindTerminalGraph_NoInheritance(Guid);
		if (!TerminalGraph ||
			!ensure(!Result.Contains(TerminalGraph)))
		{
			continue;
		}

		Result.Add_CheckNew(TerminalGraph);
	}
	return Result;
}

TVoxelSet<const UVoxelTerminalGraph*> UVoxelTerminalGraph::GetChildTerminalGraphs_LoadedOnly() const
{
	return ReinterpretCastRef<TVoxelSet<const UVoxelTerminalGraph*>>(ConstCast(this)->GetChildTerminalGraphs_LoadedOnly());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelTerminalGraph::PostLoad()
{
	Super::PostLoad();

	// Ensure Graph is fixed up first so we have a valid GUID
	GetGraph().ConditionalPostLoad();

	if (!GetGraph().FindTerminalGraphGuid_NoInheritance(this).IsValid())
	{
		// Orphan parameter graph
		return;
	}

	Fixup();
}