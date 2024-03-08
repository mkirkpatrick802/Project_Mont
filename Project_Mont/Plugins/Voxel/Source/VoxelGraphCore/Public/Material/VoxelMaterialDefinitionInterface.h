// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelObjectPinType.h"
#include "VoxelObjectWithGuid.h"
#include "Material/VoxelMaterialBlending.h"
#include "VoxelMaterialDefinitionInterface.generated.h"

class UVoxelMaterialDefinition;
class UVoxelMaterialDefinitionInstance;
class UVoxelParameterContainer_DEPRECATED;

UCLASS(Abstract, BlueprintType, meta = (AssetColor = Red))
class VOXELGRAPHCORE_API UVoxelMaterialDefinitionInterface : public UVoxelObjectWithGuid
{
	GENERATED_BODY()

public:
	virtual UVoxelMaterialDefinition* GetDefinition() const VOXEL_PURE_VIRTUAL({});
	virtual FVoxelPinValue GetParameterValue(const FGuid& Guid) const VOXEL_PURE_VIRTUAL({});

public:
	UMaterialInterface* GetPreviewMaterial();

private:
	TSharedPtr<FVoxelMaterialRef> PrivatePreviewMaterial;
	TWeakObjectPtr<UMaterialInterface> PrivatePreviewMaterialParent;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMaterialDefinitionDynamicMaterialParameter : public FVoxelDynamicMaterialParameter
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	FSimpleMulticastDelegate OnChangedMulticast;

	virtual void Apply(FName Name, UMaterialInstanceDynamic& Instance) const override;
	virtual void AddOnChanged(const FSimpleDelegate& OnChanged) override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct FVoxelMaterialParameterData : public FVoxelVirtualStruct
{
	GENERATED_BODY()
	GENERATED_VIRTUAL_STRUCT_BODY()

	struct FCachedParameters
	{
		TVoxelMap<FName, float> ScalarParameters;
		TVoxelMap<FName, FVector4> VectorParameters;
		TVoxelMap<FName, TWeakObjectPtr<UTexture>> TextureParameters;
	};
	virtual void Fixup() {}
	virtual void CacheParameters(
		FName Name,
		FCachedParameters& InOutParameters) const VOXEL_PURE_VIRTUAL();
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

constexpr int32 GVoxelMaterialDefinitionMax = 1 << 12;

class VOXELGRAPHCORE_API FVoxelMaterialDefinitionManager : public FVoxelSingleton
{
public:
	const TSharedRef<FVoxelMaterialDefinitionDynamicMaterialParameter> DynamicParameter = MakeShared<FVoxelMaterialDefinitionDynamicMaterialParameter>();

	FVoxelMaterialDefinitionManager();

	int32 Register_GameThread(UVoxelMaterialDefinitionInterface& MaterialDefinition);
	UVoxelMaterialDefinitionInterface* GetMaterialDefinition_GameThread(int32 Index);
	TWeakObjectPtr<UVoxelMaterialDefinitionInterface> GetMaterialDefinition_AnyThread(int32 Index);
	TSharedPtr<FVoxelMaterialRef> GetMaterial_AnyThread(int32 Index);

	//~ Begin FVoxelSingleton Interface
	virtual void Tick() override;
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	//~ End FVoxelSingleton Interface

	void QueueMaterialRefresh();
	void QueueRebuildTextures(UVoxelMaterialDefinition& Definition);

	void CacheParameters();
	void SetAllParameters(UMaterialInstanceDynamic& Instance);

	void LogIds();

private:
	bool bMaterialRefreshQueued = false;
	TSet<TObjectPtr<UVoxelMaterialDefinition>> MaterialDefinitionsToRebuild;

	TVoxelArray<TObjectPtr<UVoxelMaterialDefinitionInterface>> MaterialDefinitions;
	TVoxelMap<TObjectPtr<UVoxelMaterialDefinitionInterface>, int32> MaterialDefinitionToIndex;

	TUniquePtr<FVoxelMaterialParameterData::FCachedParameters> CachedParameters;

	FVoxelCriticalSection CriticalSection;
	TVoxelArray<TWeakObjectPtr<UVoxelMaterialDefinitionInterface>> WeakMaterialDefinitions_RequiresLock;
	TVoxelArray<TSharedPtr<FVoxelMaterialRef>> MaterialRefs_RequiresLock;
};

extern VOXELGRAPHCORE_API FVoxelMaterialDefinitionManager* GVoxelMaterialDefinitionManager;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_OBJECT_PIN_TYPE(FVoxelMaterialBlending);

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelMaterialBlendingPinType : public FVoxelObjectPinType
{
	GENERATED_BODY()

	DEFINE_VOXEL_OBJECT_PIN_TYPE(FVoxelMaterialBlending, UVoxelMaterialDefinitionInterface)
	{
		if (bSetObject)
		{
			Object = GVoxelMaterialDefinitionManager->GetMaterialDefinition_AnyThread(Struct.GetBestIndex());
		}
		else
		{
			const int32 Index = GVoxelMaterialDefinitionManager->Register_GameThread(*Object);
			Struct = FVoxelMaterialBlending::FromIndex(Index);
		}
	}
};