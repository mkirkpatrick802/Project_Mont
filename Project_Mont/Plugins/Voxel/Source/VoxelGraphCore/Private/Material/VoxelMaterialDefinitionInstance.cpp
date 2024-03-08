// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionInstance.h"
#include "Material/VoxelMaterialDefinition.h"
#include "VoxelParameterContainer_DEPRECATED.h"

DEFINE_VOXEL_FACTORY(UVoxelMaterialDefinitionInstance);

void UVoxelMaterialDefinitionInstance::SetParent(UVoxelMaterialDefinitionInterface* NewParent)
{
	if (Parent == NewParent)
	{
		return;
	}
	Parent = NewParent;

	if (UVoxelMaterialDefinition* Definition = GetDefinition())
	{
		Definition->QueueRebuildTextures();
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMaterialDefinitionInstance::Fixup()
{
	VOXEL_FUNCTION_COUNTER();

	UVoxelMaterialDefinition* Definition = GetDefinition();
	if (!Definition)
	{
		return;
	}

	for (auto It = GuidToValueOverride.CreateIterator(); It; ++It)
	{
		FVoxelMaterialDefinitionParameter* Parameter = Definition->GuidToMaterialParameter.Find(It.Key());
		if (!Parameter)
		{
			// Orphan
			continue;
		}

		if (!It.Value().bEnable)
		{
			const FVoxelPinValue Value = GetParameterValue(It.Key());
			if (Value == It.Value().Value)
			{
				// Disabled & same value as parent = remove
				It.RemoveCurrent();
				continue;
			}
		}

#if WITH_EDITOR
		It.Value().CachedName = Parameter->Name;
		It.Value().CachedCategory = Parameter->Category;
#endif
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UVoxelMaterialDefinition* UVoxelMaterialDefinitionInstance::GetDefinition() const
{
	TVoxelInlineSet<const UVoxelMaterialDefinitionInstance*, 8> Visited;
	for (const UVoxelMaterialDefinitionInstance* It = this; It; It = CastEnsured<UVoxelMaterialDefinitionInstance>(It->Parent))
	{
		if (Visited.Contains(It))
		{
			VOXEL_MESSAGE(Error, "Hierarchy loop detected: {0}", Visited.Array());
			return nullptr;
		}
		Visited.Add_CheckNew(It);

		if (UVoxelMaterialDefinition* Definition = Cast<UVoxelMaterialDefinition>(It->Parent))
		{
			return Definition;
		}
	}
	return nullptr;
}

FVoxelPinValue UVoxelMaterialDefinitionInstance::GetParameterValue(const FGuid& Guid) const
{
	if (const FVoxelMaterialParameterValueOverride* ValueOverride = GuidToValueOverride.Find(Guid))
	{
		return ValueOverride->Value;
	}

	if (!GetDefinition())
	{
		// No parent or loop
		return {};
	}

	return Parent->GetParameterValue(Guid);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMaterialDefinitionInstance::PostLoad()
{
	Super::PostLoad();

	for (const FVoxelParameterOverride_DEPRECATED& Parameter : ParameterCollection_DEPRECATED.Parameters)
	{
		FVoxelParameterPath Path;
		Path.Guids.Add(Parameter.Parameter.DeprecatedGuid);

		FVoxelParameterValueOverride ValueOverride;
		ValueOverride.bEnable = Parameter.bEnable;
		ValueOverride.Value = Parameter.ValueOverride;
#if WITH_EDITOR
		ValueOverride.CachedName = Parameter.Parameter.Name;
		ValueOverride.CachedCategory = Parameter.Parameter.Category;
#endif

		ensure(!ParameterOverrides_DEPRECATED.PathToValueOverride.Contains(Path));
		ParameterOverrides_DEPRECATED.PathToValueOverride.Add(Path, ValueOverride);
	}
	ParameterCollection_DEPRECATED.Parameters.Empty();

	if (ParameterContainer_DEPRECATED)
	{
		ensure(!Parent);
		Parent = CastEnsured<UVoxelMaterialDefinitionInterface>(ParameterContainer_DEPRECATED->Provider);

		ParameterOverrides_DEPRECATED.PathToValueOverride.Append(ParameterContainer_DEPRECATED->ValueOverrides);
		ParameterContainer_DEPRECATED->ValueOverrides.Empty();
	}

	for (const auto& It : ParameterOverrides_DEPRECATED.PathToValueOverride)
	{
		const FVoxelParameterPath Path = It.Key;
		if (!ensure(Path.Num() == 1))
		{
			continue;
		}

		FVoxelMaterialParameterValueOverride ValueOverride;
		ValueOverride.bEnable = It.Value.bEnable;
		ValueOverride.Value = It.Value.Value;
#if WITH_EDITORONLY_DATA
		ValueOverride.CachedName = It.Value.CachedName;
		ValueOverride.CachedCategory = It.Value.CachedCategory;
#endif

		ensure(!GuidToValueOverride.Contains(Path.Leaf()));
		GuidToValueOverride.Add(Path.Leaf(), ValueOverride);
	}
	ParameterOverrides_DEPRECATED.PathToValueOverride.Empty();

	Fixup();
}

void UVoxelMaterialDefinitionInstance::PostCDOContruct()
{
	Super::PostCDOContruct();

	FVoxelUtilities::ForceLoadDeprecatedSubobject<UVoxelParameterContainer_DEPRECATED>(this, "ParameterContainer");
}

#if WITH_EDITOR
void UVoxelMaterialDefinitionInstance::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetMemberPropertyName() == GET_OWN_MEMBER_NAME(Parent))
	{
		// Force refresh
		UVoxelMaterialDefinitionInterface* NewParent = Parent;
		Parent = nullptr;
		SetParent(NewParent);
	}

	Fixup();

	if (UVoxelMaterialDefinition* Definition = GetDefinition())
	{
		Definition->QueueRebuildTextures();
	}
}
#endif