// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinition.h"
#include "Material/VoxelMaterialDefinitionInstance.h"
#include "Material/MaterialExpressionSampleVoxelParameter.h"

DEFINE_VOXEL_FACTORY(UVoxelMaterialDefinition);

VOXEL_CONSOLE_COMMAND(
	RebuildMaterialTextures,
	"voxel.RebuildMaterialTextures",
	"")
{
	ForEachObjectOfClass<UVoxelMaterialDefinition>([&](UVoxelMaterialDefinition& Definition)
	{
		Definition.QueueRebuildTextures();
	});
}

VOXEL_CONSOLE_COMMAND(
	PurgeMaterialTextures,
	"voxel.PurgeMaterialTextures",
	"")
{
	ForEachObjectOfClass<UVoxelMaterialDefinition>([&](UVoxelMaterialDefinition& Definition)
	{
		for (auto& It : Definition.GuidToParameterData)
		{
			if (!ensure(It.Value))
			{
				continue;
			}

			for (const FProperty& Property : GetStructProperties(It.Value.GetScriptStruct()))
			{
				if (!Property.HasAnyPropertyFlags(CPF_Transient))
				{
					continue;
				}

				Property.ClearValue_InContainer(It.Value.GetPtr());
			}
		}

		Definition.QueueRebuildTextures();
	});
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMaterialDefinition::PostLoad()
{
	Super::PostLoad();

	if (GuidToParameter_DEPRECATED.Num() > 0)
	{
		for (const auto& It : GuidToParameter_DEPRECATED)
		{
			Parameters_DEPRECATED.Add(It.Value);
		}
		GuidToParameter_DEPRECATED.Empty();
	}

	for (const FVoxelParameter& Parameter : Parameters_DEPRECATED)
	{
		FVoxelMaterialDefinitionParameter MaterialDefinitionParameter;
		MaterialDefinitionParameter.Name = Parameter.Name;
		MaterialDefinitionParameter.Type = Parameter.Type;
		MaterialDefinitionParameter.DefaultValue = Parameter.DeprecatedDefaultValue;
#if WITH_EDITOR
		MaterialDefinitionParameter.Category = Parameter.Category;
		MaterialDefinitionParameter.Description = Parameter.Description;
		MaterialDefinitionParameter.MetaData = Parameter.MetaData;
#endif

		ensure(!GuidToMaterialParameter.Contains(Parameter.DeprecatedGuid));
		GuidToMaterialParameter.Add(Parameter.DeprecatedGuid, MaterialDefinitionParameter);
	}
	Parameters_DEPRECATED.Empty();

	Fixup();
}

#if WITH_EDITOR

void UVoxelMaterialDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	Fixup();

	// Fixup GuidToParameterData
	{
		TVoxelSet<FGuid> ValidGuids;
		for (const auto& It : GuidToMaterialParameter)
		{
			const FVoxelMaterialDefinitionParameter& Parameter = It.Value;
			const UMaterialExpressionSampleVoxelParameter* Template = UMaterialExpressionSampleVoxelParameter::GetTemplate(Parameter.Type);
			if (!Template)
			{
				continue;
			}

			ValidGuids.Add(It.Key);

			UScriptStruct* Struct = Template->GetVoxelParameterDataType();
			if (!ensure(Struct))
			{
				continue;
			}

			TVoxelInstancedStruct<FVoxelMaterialParameterData>& ParameterData = GuidToParameterData.FindOrAdd(It.Key);
			if (ParameterData.GetScriptStruct() != Struct)
			{
				ParameterData = FVoxelInstancedStruct(Struct);
			}
		}

		for (auto It = GuidToParameterData.CreateIterator(); It; ++It)
		{
			if (!ValidGuids.Contains(It.Key()))
			{
				It.RemoveCurrent();
				continue;
			}
			if (!It.Value())
			{
				continue;
			}
			It.Value()->Fixup();
		}
	}

	OnParametersChanged.Broadcast();
	QueueRebuildTextures();
}
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelPinValue UVoxelMaterialDefinition::GetParameterValue(const FGuid& Guid) const
{
	const FVoxelMaterialDefinitionParameter* Parameter = GuidToMaterialParameter.Find(Guid);
	if (!ensure(Parameter))
	{
		return {};
	}
	return Parameter->DefaultValue;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelMaterialDefinition::Fixup()
{
	VOXEL_FUNCTION_COUNTER();

	// Compact to avoid adding into a hole
	GuidToMaterialParameter.CompactStable();

	// Fixup parameters
	{
		TSet<FName> Names;

		for (auto& It : GuidToMaterialParameter)
		{
			FVoxelMaterialDefinitionParameter& Parameter = It.Value;
			Parameter.Fixup();

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
	}
}

void UVoxelMaterialDefinition::QueueRebuildTextures()
{
	GVoxelMaterialDefinitionManager->QueueRebuildTextures(*this);
}

void UVoxelMaterialDefinition::RebuildTextures()
{
	VOXEL_FUNCTION_COUNTER();

	if (!FApp::CanEverRender())
	{
		return;
	}

	using FInstance = UMaterialExpressionSampleVoxelParameter::FInstance;

	struct FInstances
	{
		const UMaterialExpressionSampleVoxelParameter* Template = nullptr;
		FName DebugName;
		TVoxelArray<FInstance> Instances;
	};
	TVoxelMap<FGuid, FInstances> ParameterToInstances;
	{
		VOXEL_SCOPE_COUNTER("Build instances");

		const auto AddMaterial = [&](UVoxelMaterialDefinitionInterface& Interface)
		{
			const int32 Index = GVoxelMaterialDefinitionManager->Register_GameThread(Interface);

			for (const auto& It : GuidToMaterialParameter)
			{
				const FVoxelPinValue Value = Interface.GetParameterValue(It.Key);
				if (!ensure(Value.IsValid()))
				{
					continue;
				}

				const UMaterialExpressionSampleVoxelParameter* Template = UMaterialExpressionSampleVoxelParameter::GetTemplate(It.Value.Type);
				if (!Template)
				{
					continue;
				}

				FInstances& Instances = ParameterToInstances.FindOrAdd(It.Key);
				if (!Instances.Template)
				{
					ensure(&Interface == this);
					Instances.Template = Template;
					Instances.DebugName = It.Value.Name;
				}

				Instances.Instances.Add(FInstance
				{
					&Interface,
					Index,
					Value.AsTerminalValue()
				});
			}
		};

		// Add self
		AddMaterial(*this);

		ForEachObjectOfClass<UVoxelMaterialDefinitionInstance>([&](UVoxelMaterialDefinitionInstance& Instance)
		{
			if (Instance.GetDefinition() == this)
			{
				AddMaterial(Instance);
			}
		});
	}

	TMap<FGuid, TVoxelInstancedStruct<FVoxelMaterialParameterData>> OldGuidToParameterData = MoveTemp(GuidToParameterData);
	for (auto& It : ParameterToInstances)
	{
		TVoxelInstancedStruct<FVoxelMaterialParameterData>& ParameterData = GuidToParameterData.Add(It.Key);
		if (TVoxelInstancedStruct<FVoxelMaterialParameterData>* ExistingParameterData = OldGuidToParameterData.Find(It.Key))
		{
			ParameterData = MoveTemp(*ExistingParameterData);
		}

		const FInstances& Instances = It.Value;

		UScriptStruct* Struct = Instances.Template->GetVoxelParameterDataType();
		if (!ensure(Struct))
		{
			continue;
		}
		if (!ensure(ParameterData.GetScriptStruct() == Struct))
		{
			ParameterData = FVoxelInstancedStruct(Struct);
		}

		Instances.Template->UpdateVoxelParameterData(
			Instances.DebugName,
			Instances.Instances,
			ParameterData);
	}
}