// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelLandscapeHeightmapStampFactory.generated.h"

class FVoxelLandscapeHeightmapStampData;

UCLASS()
class UVoxelLandscapeHeightmapStampFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UVoxelLandscapeHeightmapStampFactory();

	UPROPERTY(EditAnywhere, Category = "Import configuration", meta = (Units = cm))
	float ScaleZ = 25600;

	UPROPERTY(EditAnywhere, Category = "Import configuration", meta = (Units = cm, DisplayName = "Scale XY"))
	float ScaleXY = 100;

	UPROPERTY(EditAnywhere, Category = "Import", meta = (FilePathFilter = "Heightmap file (*.png; *.raw; *.r16)|*.png;*.raw;*.r16"))
	FFilePath ImportPath;

	TSharedPtr<FVoxelLandscapeHeightmapStampData> CachedHeightmap;

	//~ Begin UFactory Interface
	virtual bool ConfigureProperties() override;
	virtual FString GetDefaultNewAssetName() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	//~ End UFactory Interface

	//~ Begin FReimportHandler Interface
	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;
	//~ End FReimportHandler Interface

	static TSharedPtr<FVoxelLandscapeHeightmapStampData> Import(const FString& Path);
};

class SVoxelLandscapeHeightmapStampFactoryDetails : public SWindow
{
public:
	DECLARE_DELEGATE_RetVal(bool, FOnCreate);

	VOXEL_SLATE_ARGS()
	{
		SLATE_ARGUMENT(UObject*, DetailsObject)
		SLATE_ATTRIBUTE(bool, CanCreate)
		SLATE_EVENT(FOnCreate, OnCreate)
	};

	SVoxelLandscapeHeightmapStampFactoryDetails() = default;

	void Construct(const FArguments& Args);
};