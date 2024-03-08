// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGraph.h"
#include "VoxelActor.h"
#include "VoxelInlineGraph.h"
#include "VoxelGraphTracker.h"
#include "VoxelTerminalGraph.h"
#include "VoxelFunctionLibraryAsset.h"
#include "VoxelParameterContainer_DEPRECATED.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#if WITH_EDITOR
#include "ObjectTools.h"
#include "EdGraph/EdGraph.h"
#endif

DEFINE_VOXEL_FACTORY(UVoxelGraph);

UVoxelGraph::UVoxelGraph()
{
	if (GVoxelDoNotCreateSubobjects)
	{
		return;
	}

	UVoxelTerminalGraph* TerminalGraph = CreateDefaultSubobject<UVoxelTerminalGraph>("MainGraph");
	GuidToTerminalGraph.Add(GVoxelMainTerminalGraphGuid, TerminalGraph);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
FVoxelGraphMetadata UVoxelGraph::GetMetadata() const
{
	FVoxelGraphMetadata Metadata;
	Metadata.DisplayName = DisplayNameOverride.IsEmpty() ? GetName() : DisplayNameOverride;
	Metadata.Category = Category;
	Metadata.Description = Description;
	return Metadata;
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelTerminalGraph* UVoxelGraph::FindTerminalGraph_NoInheritance(const FGuid& Guid)
{
	return GuidToTerminalGraph.FindRef(Guid);
}

const UVoxelTerminalGraph* UVoxelGraph::FindTerminalGraph_NoInheritance(const FGuid& Guid) const
{
	return ConstCast(this)->FindTerminalGraph_NoInheritance(Guid);
}

void UVoxelGraph::ForeachTerminalGraph_NoInheritance(TFunctionRef<void(UVoxelTerminalGraph&)> Lambda)
{
	for (const auto& It : GuidToTerminalGraph)
	{
		check(It.Value);
		Lambda(*It.Value);
	}
}

void UVoxelGraph::ForeachTerminalGraph_NoInheritance(TFunctionRef<void(const UVoxelTerminalGraph&)> Lambda) const
{
	ConstCast(this)->ForeachTerminalGraph_NoInheritance(ReinterpretCastRef<TFunctionRef<void(UVoxelTerminalGraph&)>>(Lambda));
}

FGuid UVoxelGraph::FindTerminalGraphGuid_NoInheritance(const UVoxelTerminalGraph* TerminalGraph) const
{
	VOXEL_FUNCTION_COUNTER();

	FGuid Result;
	for (const auto& It : GuidToTerminalGraph)
	{
		if (It.Value == TerminalGraph)
		{
			ensure(!Result.IsValid());
			Result = It.Key;
		}
	}
	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelTerminalGraph* UVoxelGraph::FindTerminalGraph(const FGuid& Guid)
{
	for (const UVoxelGraph* BaseGraph : GetBaseGraphs())
	{
		if (UVoxelTerminalGraph* TerminalGraph = BaseGraph->GuidToTerminalGraph.FindRef(Guid))
		{
			return TerminalGraph;
		}
	}
	return nullptr;
}

const UVoxelTerminalGraph* UVoxelGraph::FindTerminalGraph(const FGuid& Guid) const
{
	return ConstCast(this)->FindTerminalGraph(Guid);
}

UVoxelTerminalGraph& UVoxelGraph::FindTerminalGraphChecked(const FGuid& Guid)
{
	UVoxelTerminalGraph* TerminalGraph = FindTerminalGraph(Guid);
	check(TerminalGraph);
	return *TerminalGraph;
}

const UVoxelTerminalGraph& UVoxelGraph::FindTerminalGraphChecked(const FGuid& Guid) const
{
	return ConstCast(this)->FindTerminalGraphChecked(Guid);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TVoxelSet<FGuid> UVoxelGraph::GetTerminalGraphs() const
{
	TVoxelSet<FGuid> Result;
	for (const UVoxelGraph* BaseGraph : GetBaseGraphs())
	{
		for (const auto& It : BaseGraph->GuidToTerminalGraph)
		{
			if (!ensure(It.Value))
			{
				continue;
			}

			Result.Add(It.Key);
		}
	}
	return Result;
}

bool UVoxelGraph::IsTerminalGraphOverrideable(const FGuid& Guid) const
{
	ensure(Guid != GVoxelMainTerminalGraphGuid);

	UVoxelGraph* BaseGraph = GetBaseGraph_Unsafe();
	if (BaseGraph == this)
	{
		return false;
	}

	return BaseGraph->FindTerminalGraph(Guid) != nullptr;
}

const UVoxelTerminalGraph* UVoxelGraph::FindTopmostTerminalGraph(const FGuid& Guid) const
{
	if (!ensure(Guid != GVoxelMainTerminalGraphGuid))
	{
		return nullptr;
	}

	const UVoxelGraph* LastBaseGraph = this;
	for (const UVoxelGraph* BaseGraph : GetBaseGraphs())
	{
		if (!BaseGraph->FindTerminalGraph(Guid))
		{
			break;
		}

		LastBaseGraph = BaseGraph;
	}

	const UVoxelTerminalGraph* TerminalGraph = LastBaseGraph->FindTerminalGraph(Guid);
	ensure(TerminalGraph);
	return TerminalGraph;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
UVoxelTerminalGraph& UVoxelGraph::AddTerminalGraph(
	const FGuid& Guid,
	const UVoxelTerminalGraph* Template)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(!GuidToTerminalGraph.Contains(Guid)))
	{
		return *GuidToTerminalGraph[Guid];
	}
	ensure(!FindTerminalGraphGuid_NoInheritance(Template).IsValid());

	UVoxelTerminalGraph* NewTerminalGraph;
	if (Template)
	{
		ensure(!Template->HasAnyFlags(RF_ClassDefaultObject));
		NewTerminalGraph = DuplicateObject<UVoxelTerminalGraph>(Template, this, NAME_None);
	}
	else
	{
		NewTerminalGraph = NewObject<UVoxelTerminalGraph>(this, NAME_None, RF_Transactional);
	}
	NewTerminalGraph->SetGuid_Hack(Guid);

	GuidToTerminalGraph.Add(Guid, NewTerminalGraph);
	GVoxelGraphTracker->NotifyTerminalGraphChanged(*this);

	return *NewTerminalGraph;
}

void UVoxelGraph::RemoveTerminalGraph(const FGuid& Guid)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Guid != GVoxelMainTerminalGraphGuid))
	{
		return;
	}

	ensure(GuidToTerminalGraph.Remove(Guid));

	GVoxelGraphTracker->NotifyTerminalGraphChanged(*this);
}

void UVoxelGraph::ReorderTerminalGraphs(const TConstVoxelArrayView<FGuid> NewGuids)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FGuid> FinalGuids;
	for (const FGuid& Guid : NewGuids)
	{
		ensure(FindTerminalGraph(Guid));

		if (GuidToTerminalGraph.Contains(Guid))
		{
			FinalGuids.Add(Guid);
		}
	}
	FVoxelUtilities::ReorderMapKeys(GuidToTerminalGraph, FinalGuids);

	GVoxelGraphTracker->NotifyTerminalGraphChanged(*this);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const FVoxelParameter* UVoxelGraph::FindParameter(const FGuid& Guid) const
{
	for (const UVoxelGraph* BaseGraph : GetBaseGraphs())
	{
		if (const FVoxelParameter* Parameter = BaseGraph->GuidToParameter.Find(Guid))
		{
			return Parameter;
		}
	}
	return nullptr;
}

const FVoxelParameter& UVoxelGraph::FindParameterChecked(const FGuid& Guid) const
{
	const FVoxelParameter* Parameter = FindParameter(Guid);
	check(Parameter);
	return *Parameter;
}

TVoxelSet<FGuid> UVoxelGraph::GetParameters() const
{
	TVoxelSet<FGuid> Result;
	// Add parent parameters first
	for (const UVoxelGraph* BaseGraph : GetBaseGraphs().Reverse())
	{
		for (const auto& It : BaseGraph->GuidToParameter)
		{
			ensure(!Result.Contains(It.Key));
			Result.Add(It.Key);
		}
	}
	return Result;
}

bool UVoxelGraph::IsInheritedParameter(const FGuid& Guid) const
{
	return
		ensure(FindParameter(Guid)) &&
		!GuidToParameter.Contains(Guid);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
void UVoxelGraph::AddParameter(const FGuid& Guid, const FVoxelParameter& Parameter)
{
	VOXEL_FUNCTION_COUNTER();
	ensure(!FindParameter(Guid));

	GuidToParameter.Add(Guid, Parameter);

	GVoxelGraphTracker->NotifyParameterChanged(*this);
}

void UVoxelGraph::RemoveParameter(const FGuid& Guid)
{
	VOXEL_FUNCTION_COUNTER();

	ensure(GuidToParameter.Remove(Guid));

	GVoxelGraphTracker->NotifyParameterChanged(*this);
}

void UVoxelGraph::UpdateParameter(const FGuid& Guid, TFunctionRef<void(FVoxelParameter&)> Update)
{
	VOXEL_FUNCTION_COUNTER();

	FVoxelParameter* Parameter = GuidToParameter.Find(Guid);
	if (!ensure(Parameter))
	{
		return;
	}
	const FVoxelParameter PreviousParameter = *Parameter;

	Update(*Parameter);

	// If category changed move the parameter to the end to avoid reordering an entire category as a side effect
	if (PreviousParameter.Category != Parameter->Category)
	{
		FVoxelParameter ParameterCopy;
		verify(GuidToParameter.RemoveAndCopyValue(Guid, ParameterCopy));
		GuidToParameter.CompactStable();
		GuidToParameter.Add(Guid, ParameterCopy);
	}

	GVoxelGraphTracker->NotifyParameterChanged(*this);
}

void UVoxelGraph::ReorderParameters(const TConstVoxelArrayView<FGuid> NewGuids)
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelArray<FGuid> FinalGuids;
	for (const FGuid& Guid : NewGuids)
	{
		ensure(FindParameter(Guid));

		if (GuidToParameter.Contains(Guid))
		{
			FinalGuids.Add(Guid);
		}
	}
	FVoxelUtilities::ReorderMapKeys(GuidToParameter, FinalGuids);

	GVoxelGraphTracker->NotifyParameterChanged(*this);
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraph::Fixup()
{
	VOXEL_FUNCTION_COUNTER();

#if WITH_EDITOR
	if (!bEnableThumbnail)
	{
		ThumbnailTools::CacheEmptyThumbnail(GetFullName(), GetPackage());
	}
#endif

	// Before anything else, fix GUIDs
	for (auto It = GuidToTerminalGraph.CreateIterator(); It; ++It)
	{
		UVoxelTerminalGraph* TerminalGraph = It.Value();
		if (!ensure(TerminalGraph))
		{
			It.RemoveCurrent();
			continue;
		}

		TerminalGraph->SetGuid_Hack(It.Key());
	}

	// Fixup terminal graphs
	ForeachTerminalGraph_NoInheritance([&](UVoxelTerminalGraph& TerminalGraph)
	{
		TerminalGraph.ConditionalPostLoad();
		TerminalGraph.Fixup();
	});

#if WITH_EDITOR
	// Make function names unique
	{
		TSet<FString> UsedNames;
		ForeachTerminalGraph_NoInheritance([&](UVoxelTerminalGraph& TerminalGraph)
		{
			if (TerminalGraph.IsMainTerminalGraph() ||
				!TerminalGraph.IsTopmostTerminalGraph())
			{
				return;
			}

			FString DisplayName = TerminalGraph.GetDisplayName();
			while (UsedNames.Contains(DisplayName))
			{
				FName DisplayFName(DisplayName);
				DisplayFName.SetNumber(DisplayFName.GetNumber() + 1);
				DisplayName = DisplayFName.ToString();

				TerminalGraph.UpdateMetadata([&](FVoxelGraphMetadata& Metadata)
				{
					Metadata.DisplayName = DisplayName;
				});
			}
			UsedNames.Add(DisplayName);
		});
	}
#endif

	// Compact to avoid adding into a hole
	GuidToTerminalGraph.CompactStable();
	GuidToParameter.CompactStable();

	// Fixup buffer types
	for (auto& It : GuidToParameter)
	{
		FVoxelParameter& Parameter = It.Value;
		if (!Parameter.Type.IsBuffer())
		{
			continue;
		}
		if (Parameter.Type.IsBufferArray())
		{
			continue;
		}

		ensure(false);
		Parameter.Type = Parameter.Type.GetInnerType();
	}

	// Fixup parameters
	{
		TSet<FName> Names;

		for (auto& It : GuidToParameter)
		{
			FVoxelParameter& Parameter = It.Value;
			Parameter.Fixup();
#if WITH_EDITOR
			Parameter.Category = FVoxelUtilities::SanitizeCategory(Parameter.Category);
#endif

			if (Parameter.Name.IsNone())
			{
				Parameter.Name = "Parameter";
			}

			while (Names.Contains(Parameter.Name))
			{
				Parameter.Name.SetNumber(Parameter.Name.GetNumber() + 1);
			}

			Names.Add(Parameter.Name);
		}

#if WITH_EDITOR
		// Sort parameters by category
		TMap<FString, TMap<FGuid, FVoxelParameter>> CategoryToGuidToParameter;
		for (const auto& It : GuidToParameter)
		{
			CategoryToGuidToParameter.FindOrAdd(It.Value.Category).Add(It.Key, It.Value);
		}

		GuidToParameter.Empty();
		for (const auto& CategoryIt : CategoryToGuidToParameter)
		{
			for (const auto& It : CategoryIt.Value)
			{
				GuidToParameter.Add(It.Key, It.Value);
			}
		}
#endif
	}

	FixupParameterOverrides();
}

bool UVoxelGraph::IsFunctionLibrary() const
{
	const bool bIsFunctionLibrary = GetTypedOuter<UVoxelFunctionLibraryAsset>() != nullptr;
	if (bIsFunctionLibrary)
	{
		ensure(!PrivateBaseGraph);
	}
	return bIsFunctionLibrary;
}

UVoxelTerminalGraph& UVoxelGraph::GetMainTerminalGraph()
{
	ensure(!IsFunctionLibrary());
	TObjectPtr<UVoxelTerminalGraph>& MainGraph = GuidToTerminalGraph.FindOrAdd(GVoxelMainTerminalGraphGuid);
	if (!ensure(MainGraph))
	{
		MainGraph = NewObject<UVoxelTerminalGraph>(this, "MainGraph", RF_Transactional);
	}
	check(MainGraph);
	return *MainGraph;
}

const UVoxelTerminalGraph& UVoxelGraph::GetMainTerminalGraph() const
{
	return ConstCast(this)->GetMainTerminalGraph();
}

void UVoxelGraph::LoadAllGraphs()
{
	VOXEL_FUNCTION_COUNTER();

	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	TArray<FAssetData> AssetDatas;

	FARFilter Filter;
	Filter.ClassPaths.Add(StaticClassFast<UVoxelGraph>()->GetClassPathName());
	Filter.ClassPaths.Add(StaticClassFast<UVoxelFunctionLibraryAsset>()->GetClassPathName());
	AssetRegistryModule.Get().GetAssets(Filter, AssetDatas);

	for (const FAssetData& AssetData : AssetDatas)
	{
		(void)AssetData.GetAsset();
	}
}

TVoxelSet<const UVoxelGraph*> UVoxelGraph::GetUsedGraphs() const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelSet<const UVoxelGraph*> Result;
	GetUsedGraphsImpl(Result);
	return Result;
}

void UVoxelGraph::GetUsedGraphsImpl(TVoxelSet<const UVoxelGraph*>& OutGraphs) const
{
	if (OutGraphs.Contains(this))
	{
		return;
	}
	OutGraphs.Add(this);

	for (const auto& It : ParameterOverrides.PathToValueOverride)
	{
		if (!It.Value.Value.Is<UVoxelGraph>())
		{
			continue;
		}

		const UVoxelGraph* Graph = It.Value.Value.Get<UVoxelGraph>();
		if (!Graph)
		{
			continue;
		}

		Graph->GetUsedGraphsImpl(OutGraphs);
	}

	for (const UVoxelGraph* BaseGraph : GetBaseGraphs())
	{
		BaseGraph->GetUsedGraphsImpl(OutGraphs);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelGraph* UVoxelGraph::GetBaseGraph_Unsafe() const
{
	if (!ensure(PrivateBaseGraph != this))
	{
		return nullptr;
	}
	if (PrivateBaseGraph)
	{
		ensure(!IsFunctionLibrary());
	}
	return PrivateBaseGraph;
}

#if WITH_EDITOR
void UVoxelGraph::SetBaseGraph(UVoxelGraph* NewBaseGraph)
{
	ensure(!IsFunctionLibrary());

	if (PrivateBaseGraph == NewBaseGraph)
	{
		return;
	}
	PrivateBaseGraph = NewBaseGraph;

	GVoxelGraphTracker->NotifyBaseGraphChanged(*this);
}
#endif

TVoxelInlineSet<UVoxelGraph*, 8> UVoxelGraph::GetBaseGraphs()
{
	TVoxelInlineSet<UVoxelGraph*, 8> Result;
	for (UVoxelGraph* Graph = this; Graph; Graph = Graph->GetBaseGraph_Unsafe())
	{
		if (Result.Contains(Graph))
		{
			VOXEL_MESSAGE(Error, "Hierarchy loop detected: {0}", Result.Array());
			return Result;
		}
		Result.Add_CheckNew(Graph);
	}
	return Result;
}

TVoxelInlineSet<const UVoxelGraph*, 8> UVoxelGraph::GetBaseGraphs() const
{
	return ReinterpretCastRef<TVoxelInlineSet<const UVoxelGraph*, 8>>(ConstCast(this)->GetBaseGraphs());
}

TVoxelSet<UVoxelGraph*> UVoxelGraph::GetChildGraphs_LoadedOnly()
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelSet<UVoxelGraph*> Result;
	Result.Reserve(64);
	ForEachObjectOfClass<UVoxelGraph>([&](UVoxelGraph& Graph)
	{
		if (Graph.GetBaseGraphs().Contains(this))
		{
			Result.Add(&Graph);
		}
	});
	return Result;
}

TVoxelSet<const UVoxelGraph*> UVoxelGraph::GetChildGraphs_LoadedOnly() const
{
	return ReinterpretCastRef<TVoxelSet<const UVoxelGraph*>>(ConstCast(this)->GetChildGraphs_LoadedOnly());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGraph::PostLoad()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostLoad();

#if WITH_EDITOR
	if (Graph_DEPRECATED)
	{
		FObjectDuplicationParameters DuplicationParameters(Graph_DEPRECATED, GetOuter());
		DuplicationParameters.DuplicationSeed.Add(Graph_DEPRECATED, this);
		StaticDuplicateObjectEx(DuplicationParameters);
	}

	if (!MainEdGraph_DEPRECATED)
	{
		MainEdGraph_DEPRECATED = Graphs_DEPRECATED.FindRef("Main");
	}

	if (MainEdGraph_DEPRECATED)
	{
		MainEdGraph_DEPRECATED->ConditionalPostLoad();
		MainEdGraph_DEPRECATED->ClearFlags(RF_DefaultSubObject);

		UVoxelTerminalGraph& TerminalGraph = GetMainTerminalGraph();

		// REN_ForceNoResetLoaders is needed in PostLoad, otherwise loading hangs
		verify(MainEdGraph_DEPRECATED->Rename(nullptr, &TerminalGraph, REN_ForceNoResetLoaders));
		TerminalGraph.SetEdGraph_Hack(MainEdGraph_DEPRECATED);
		TerminalGraph.SetDisplayName_Hack(NameOverride_DEPRECATED);
		TerminalGraph.bExposeToLibrary = bExposeToLibrary_DEPRECATED;

		for (FVoxelGraphParameter_DEPRECATED& Parameter : Parameters_DEPRECATED)
		{
			// Migrate channel names
			Parameter.Fixup();

			switch (Parameter.ParameterType)
			{
			default: ensure(false);
			case EVoxelGraphParameterType_DEPRECATED::Parameter:
			{
				FVoxelParameter SanitizedParameter = Parameter;
				SanitizedParameter.DeprecatedGuid = {};
				SanitizedParameter.DeprecatedDefaultValue = {};

				ensure(!GuidToParameter.Contains(Parameter.DeprecatedGuid));
				GuidToParameter.Add(Parameter.DeprecatedGuid, SanitizedParameter);

				if (!Parameter.DeprecatedDefaultValue.Is<FVoxelInlineGraph>())
				{
					ensure(SetParameter(Parameter.Name, Parameter.DeprecatedDefaultValue));
				}
			}
			break;
			case EVoxelGraphParameterType_DEPRECATED::Input:
			{
				FVoxelGraphInput Input;
				Input.Name = Parameter.Name;
				Input.Type = Parameter.Type;
				Input.Category = Parameter.Category;
				Input.Description = Parameter.Description;
				Input.DefaultValue = Parameter.DeprecatedDefaultValue;
				Input.bDeprecatedExposeInputDefaultAsPin = Parameter.bExposeInputDefaultAsPin_DEPRECATED;

				ensure(!TerminalGraph.FindInput(Parameter.DeprecatedGuid));
				TerminalGraph.AddInput(Parameter.DeprecatedGuid, Input);
			}
			break;
			case EVoxelGraphParameterType_DEPRECATED::Output:
			{
				FVoxelGraphOutput Output;
				Output.Name = Parameter.Name;
				Output.Type = Parameter.Type;
				Output.Category = Parameter.Category;
				Output.Description = Parameter.Description;

				ensure(!TerminalGraph.FindOutput(Parameter.DeprecatedGuid));
				TerminalGraph.AddOutput(Parameter.DeprecatedGuid, Output);
			}
			break;
			case EVoxelGraphParameterType_DEPRECATED::LocalVariable:
			{
				FVoxelGraphLocalVariable LocalVariable;
				LocalVariable.Name = Parameter.Name;
				LocalVariable.Type = Parameter.Type;
				LocalVariable.Category = Parameter.Category;
				LocalVariable.Description = Parameter.Description;
				LocalVariable.DeprecatedDefaultValue = Parameter.DeprecatedDefaultValue;

				ensure(!TerminalGraph.FindLocalVariable(Parameter.DeprecatedGuid));
				TerminalGraph.AddLocalVariable(Parameter.DeprecatedGuid, LocalVariable);
			}
			break;
			}
		}
		Parameters_DEPRECATED.Empty();
	}
	ensure(Parameters_DEPRECATED.Num() == 0);

	for (UVoxelGraph* InlineMacro : InlineMacros_DEPRECATED)
	{
		if (!InlineMacro)
		{
			continue;
		}
		InlineMacro->ConditionalPostLoad();

		UVoxelTerminalGraph& TerminalGraph = InlineMacro->GetMainTerminalGraph();
		// REN_ForceNoResetLoaders is needed in PostLoad, otherwise loading hangs
		TerminalGraph.Rename(nullptr, this, REN_ForceNoResetLoaders);
		TerminalGraph.ClearFlags(RF_DefaultSubObject);

		if (!ensure(!FindTerminalGraphGuid_NoInheritance(&TerminalGraph).IsValid()))
		{
			continue;
		}

		GuidToTerminalGraph.Add(FGuid::NewGuid(), &TerminalGraph);
	}

	if (Parent_DEPRECATED)
	{
		SetBaseGraph(Parent_DEPRECATED.Get());
		ParameterCollection_DEPRECATED.MigrateTo(*this);
	}

	if (ParameterContainer_DEPRECATED)
	{
		ensure(!GetBaseGraph_Unsafe());
		SetBaseGraph(CastEnsured<UVoxelGraph>(ParameterContainer_DEPRECATED->Provider));

		ParameterContainer_DEPRECATED->MigrateTo(ParameterOverrides);
	}
#endif

	Fixup();
}

void UVoxelGraph::PostInitProperties()
{
	VOXEL_FUNCTION_COUNTER();

	Super::PostInitProperties();

	FixupParameterOverrides();
}

void UVoxelGraph::PostCDOContruct()
{
	Super::PostCDOContruct();

	FVoxelUtilities::ForceLoadDeprecatedSubobject<UVoxelGraph>(this, "Graph");
	FVoxelUtilities::ForceLoadDeprecatedSubobject<UVoxelParameterContainer_DEPRECATED>(this, "ParameterContainer");
}

#if WITH_EDITOR
void UVoxelGraph::PostEditUndo()
{
	Super::PostEditUndo();

	FixupParameterOverrides();
}

void UVoxelGraph::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(Category) ||
		PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(Description) ||
		PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(DisplayNameOverride))
	{
		GVoxelGraphTracker->NotifyTerminalGraphMetaDataChanged(GetMainTerminalGraph());
	}
}

void UVoxelGraph::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);

	GVoxelGraphTracker->NotifyTerminalGraphMetaDataChanged(GetMainTerminalGraph());
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelGraph::ShouldForceEnableOverride(const FVoxelParameterPath& Path) const
{
	// Force enable our own parameters
	return
		Path.Num() == 1 &&
		GuidToParameter.Contains(Path.Leaf());
}

FVoxelParameterOverrides& UVoxelGraph::GetParameterOverrides()
{
	return ParameterOverrides;
}