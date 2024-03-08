// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Material/VoxelMaterialDefinitionInterface.h"
#include "Material/VoxelMaterialDefinition.h"
#include "MaterialDomain.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

VOXEL_CONSOLE_COMMAND(
	LogVoxelMaterialDefinitionIds,
	"voxel.material.LogIds",
	"")
{
	GVoxelMaterialDefinitionManager->LogIds();
}

FVoxelMaterialDefinitionManager* GVoxelMaterialDefinitionManager = new FVoxelMaterialDefinitionManager();

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UMaterialInterface* UVoxelMaterialDefinitionInterface::GetPreviewMaterial()
{
	const UVoxelMaterialDefinition* Definition = GetDefinition();
	if (!Definition)
	{
		return UMaterial::GetDefaultMaterial(MD_Surface);
	}
	if (!PrivatePreviewMaterial ||
		PrivatePreviewMaterialParent != Definition->Material)
	{
		PrivatePreviewMaterial = FVoxelMaterialRef::MakeInstance(Definition->Material);
		PrivatePreviewMaterialParent = Definition->Material;

		PrivatePreviewMaterial->SetScalarParameter_GameThread(
			"PreviewMaterialId",
			GVoxelMaterialDefinitionManager->Register_GameThread(*this));

		PrivatePreviewMaterial->SetDynamicParameter_GameThread(
			"GVoxelMaterialDefinitionManager",
			GVoxelMaterialDefinitionManager->DynamicParameter);
	}

	return PrivatePreviewMaterial->GetMaterial();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionDynamicMaterialParameter::Apply(const FName Name, UMaterialInstanceDynamic& Instance) const
{
	GVoxelMaterialDefinitionManager->SetAllParameters(Instance);
}

void FVoxelMaterialDefinitionDynamicMaterialParameter::AddOnChanged(const FSimpleDelegate & OnChanged)
{
	OnChangedMulticast.Add(OnChanged);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FVoxelMaterialDefinitionManager::FVoxelMaterialDefinitionManager()
{
	MaterialDefinitions.Add(nullptr);
	WeakMaterialDefinitions_RequiresLock.Add(nullptr);
	MaterialRefs_RequiresLock.Add(nullptr);
}

int32 FVoxelMaterialDefinitionManager::Register_GameThread(UVoxelMaterialDefinitionInterface& MaterialDefinition)
{
	check(IsInGameThread());

	if (const int32* IndexPtr = MaterialDefinitionToIndex.Find(&MaterialDefinition))
	{
		return *IndexPtr;
	}

	VOXEL_FUNCTION_COUNTER();

	const int32 NewIndex = MaterialDefinitions.Num();

	if (NewIndex == GVoxelMaterialDefinitionMax - 1)
	{
		VOXEL_MESSAGE(Error, "Max number of voxel materials reached");
	}
	else
	{
		ensure(MaterialDefinitions.Num() < GVoxelMaterialDefinitionMax);
		MaterialDefinitions.Add(&MaterialDefinition);

		VOXEL_SCOPE_LOCK(CriticalSection);
		WeakMaterialDefinitions_RequiresLock.Add(&MaterialDefinition);

		if (const UVoxelMaterialDefinition* Definition = MaterialDefinition.GetDefinition())
		{
			MaterialRefs_RequiresLock.Add(FVoxelMaterialRef::Make(Definition->Material));
		}
		else
		{
			MaterialRefs_RequiresLock.Add(nullptr);
		}
	}

	MaterialDefinitionToIndex.Add_CheckNew(&MaterialDefinition, NewIndex);
	bMaterialRefreshQueued = true;

	if (UVoxelMaterialDefinition* Definition = MaterialDefinition.GetDefinition())
	{
		MaterialDefinitionsToRebuild.Add(Definition);
	}

	return NewIndex;
}

UVoxelMaterialDefinitionInterface* FVoxelMaterialDefinitionManager::GetMaterialDefinition_GameThread(const int32 Index)
{
	check(IsInGameThread());

	if (!MaterialDefinitions.IsValidIndex(Index))
	{
		return nullptr;
	}

	return MaterialDefinitions[Index];
}

TWeakObjectPtr<UVoxelMaterialDefinitionInterface> FVoxelMaterialDefinitionManager::GetMaterialDefinition_AnyThread(const int32 Index)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	checkVoxelSlow(MaterialDefinitions.Num() == WeakMaterialDefinitions_RequiresLock.Num());
	checkVoxelSlow(MaterialDefinitions.Num() == MaterialRefs_RequiresLock.Num());

	if (!WeakMaterialDefinitions_RequiresLock.IsValidIndex(Index))
	{
		return nullptr;
	}

	const TWeakObjectPtr<UVoxelMaterialDefinitionInterface> Result = WeakMaterialDefinitions_RequiresLock[Index];
	checkVoxelSlow(!Result.IsValid() || Result == MaterialDefinitions[Index]);
	return Result;
}

TSharedPtr<FVoxelMaterialRef> FVoxelMaterialDefinitionManager::GetMaterial_AnyThread(const int32 Index)
{
	VOXEL_SCOPE_LOCK(CriticalSection);

	checkVoxelSlow(MaterialDefinitions.Num() == WeakMaterialDefinitions_RequiresLock.Num());
	checkVoxelSlow(MaterialDefinitions.Num() == MaterialRefs_RequiresLock.Num());

	if (!MaterialRefs_RequiresLock.IsValidIndex(Index))
	{
		return nullptr;
	}

	const TSharedPtr<FVoxelMaterialRef> Result = MaterialRefs_RequiresLock[Index];

#if VOXEL_DEBUG
	if (const UVoxelMaterialDefinitionInterface* MaterialDefinition = MaterialDefinitions[Index])
	{
		if (const UVoxelMaterialDefinition* Definition = MaterialDefinition->GetDefinition())
		{
			ensureVoxelSlow(
				!Result.IsValid() ||
				!Definition->Material ||
				Result->GetMaterial() == Definition->Material);
		}
	}
#endif

	return Result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionManager::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	if (bMaterialRefreshQueued)
	{
		bMaterialRefreshQueued = false;
		CacheParameters();

		VOXEL_SCOPE_COUNTER("OnChangedMulticast");
		DynamicParameter->OnChangedMulticast.Broadcast();
	}

	const TSet<TObjectPtr<UVoxelMaterialDefinition>> MaterialDefinitionsToRebuildCopy = MoveTemp(MaterialDefinitionsToRebuild);
	check(MaterialDefinitionsToRebuild.Num() == 0);

	for (UVoxelMaterialDefinition* Definition : MaterialDefinitionsToRebuildCopy)
	{
		if (!ensure(Definition))
		{
			continue;
		}

		Definition->RebuildTextures();

		const int32* IndexPtr = MaterialDefinitionToIndex.Find(Definition);
		if (!ensure(IndexPtr))
		{
			continue;
		}

		VOXEL_SCOPE_LOCK(CriticalSection);

		if (!ensure(MaterialRefs_RequiresLock.IsValidIndex(*IndexPtr)))
		{
			continue;
		}

		// TODO Update instances as well
		// TODO Dependency for auto refresh
		MaterialRefs_RequiresLock[*IndexPtr] = FVoxelMaterialRef::Make(Definition->Material);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionManager::AddReferencedObjects(FReferenceCollector& Collector)
{
	VOXEL_FUNCTION_COUNTER();

	Collector.AddReferencedObjects(MaterialDefinitionsToRebuild);

	for (TObjectPtr<UVoxelMaterialDefinitionInterface>& Material : MaterialDefinitions)
	{
		Collector.AddReferencedObject(Material);
	}

	for (auto It = MaterialDefinitionToIndex.CreateIterator(); It; ++It)
	{
		TObjectPtr<UVoxelMaterialDefinitionInterface> Object = It.Key();
		Collector.AddReferencedObject(Object);

		if (!ensureVoxelSlow(Object))
		{
			It.RemoveCurrent();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionManager::QueueMaterialRefresh()
{
	bMaterialRefreshQueued = true;
}

void FVoxelMaterialDefinitionManager::QueueRebuildTextures(UVoxelMaterialDefinition& Definition)
{
	MaterialDefinitionsToRebuild.Add(&Definition);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionManager::CacheParameters()
{
	VOXEL_FUNCTION_COUNTER();

	CachedParameters = MakeUnique<FVoxelMaterialParameterData::FCachedParameters>();

	for (UVoxelMaterialDefinitionInterface* Material : MaterialDefinitions)
	{
		UVoxelMaterialDefinition* MaterialDefinition = Cast<UVoxelMaterialDefinition>(Material);
		if (!MaterialDefinition)
		{
			continue;
		}

		const FString BaseName = "VOXELPARAM_" + MaterialDefinition->GetGuid().ToString() + "_";
		for (const auto& It : MaterialDefinition->GuidToParameterData)
		{
			It.Value->CacheParameters(
				FName(BaseName + It.Key.ToString()),
				*CachedParameters);
		}
	}
}

void FVoxelMaterialDefinitionManager::SetAllParameters(UMaterialInstanceDynamic& Instance)
{
	VOXEL_FUNCTION_COUNTER();

	if (!CachedParameters)
	{
		CacheParameters();
	}

	for (const auto& It : CachedParameters->ScalarParameters)
	{
		VOXEL_SCOPE_COUNTER("SetScalarParameterValue");
		Instance.SetScalarParameterValue(It.Key, It.Value);
	}
	for (const auto& It : CachedParameters->VectorParameters)
	{
		VOXEL_SCOPE_COUNTER("SetVectorParameterValue");
		Instance.SetVectorParameterValue(It.Key, It.Value);
	}
	for (const auto& It : CachedParameters->TextureParameters)
	{
		VOXEL_SCOPE_COUNTER("SetTextureParameterValue");
		Instance.SetTextureParameterValue(It.Key, It.Value.Get());
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelMaterialDefinitionManager::LogIds()
{
	VOXEL_FUNCTION_COUNTER();
	check(IsInGameThread());

	for (int32 Index = 0; Index < MaterialDefinitions.Num(); Index++)
	{
		if (const UVoxelMaterialDefinitionInterface* Material = MaterialDefinitions[Index])
		{
			LOG_VOXEL(Log, "%d: %s", Index, *Material->GetPathName());
		}
		else
		{
			LOG_VOXEL(Log, "%d: null", Index);
		}
	}
}