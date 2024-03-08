// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelLandscapeHeightmapStampFactory.h"
#include "VoxelHeightmapImporter.h"
#include "Height/Stamp/VoxelLandscapeHeightmapStampAsset.h"

UVoxelLandscapeHeightmapStampFactory::UVoxelLandscapeHeightmapStampFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	bEditorImport = true;
	SupportedClass = UVoxelLandscapeHeightmapStampAsset::StaticClass();
}

bool UVoxelLandscapeHeightmapStampFactory::ConfigureProperties()
{
	// Load from default
	UVoxelLandscapeHeightmapStampFactory* Default = GetMutableDefault<UVoxelLandscapeHeightmapStampFactory>();
	ScaleZ = Default->ScaleZ;
	ScaleXY = Default->ScaleXY;
	ImportPath = Default->ImportPath;

	ON_SCOPE_EXIT
	{
		// Save to default
		Default->ScaleZ = ScaleZ;
		Default->ScaleXY = ScaleXY;
		Default->ImportPath = ImportPath;
	};

	const TSharedRef<SVoxelLandscapeHeightmapStampFactoryDetails> PickerWindow =
		SNew(SVoxelLandscapeHeightmapStampFactoryDetails)
		.DetailsObject(this)
		.CanCreate_Lambda([=]
		{
			return !ImportPath.FilePath.IsEmpty();
		})
		.OnCreate_Lambda([=]
		{
			CachedHeightmap = Import(ImportPath.FilePath);
			return CachedHeightmap.IsValid();
		});

	GEditor->EditorAddModalWindow(PickerWindow);

	return CachedHeightmap.IsValid();
}

FString UVoxelLandscapeHeightmapStampFactory::GetDefaultNewAssetName() const
{
	return FPaths::GetBaseFilename(ImportPath.FilePath);
}

UObject* UVoxelLandscapeHeightmapStampFactory::FactoryCreateNew(UClass* Class, UObject* InParent, const FName Name, const EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	if (!ensure(CachedHeightmap))
	{
		return nullptr;
	}

	UVoxelLandscapeHeightmapStampAsset* Asset = NewObject<UVoxelLandscapeHeightmapStampAsset>(InParent, Class, Name, Flags);
	Asset->ScaleZ = ScaleZ;
	Asset->ScaleXY = ScaleXY;
	Asset->ImportPath = ImportPath;
	Asset->Import(CachedHeightmap.ToSharedRef());

	CachedHeightmap.Reset();

	return Asset;
}

bool UVoxelLandscapeHeightmapStampFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	const UVoxelLandscapeHeightmapStampAsset* Asset = Cast<UVoxelLandscapeHeightmapStampAsset>(Obj);
	if (!Asset)
	{
		// Can happen
		return false;
	}

	OutFilenames.Add(Asset->ImportPath.FilePath);

	return true;
}

void UVoxelLandscapeHeightmapStampFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UVoxelLandscapeHeightmapStampAsset* Asset = Cast<UVoxelLandscapeHeightmapStampAsset>(Obj);
	if (!ensure(Asset))
	{
		return;
	}

	if (!ensure(NewReimportPaths.Num() == 1))
	{
		return;
	}

	Asset->ImportPath.FilePath = NewReimportPaths[0];
}

EReimportResult::Type UVoxelLandscapeHeightmapStampFactory::Reimport(UObject* Obj)
{
	UVoxelLandscapeHeightmapStampAsset* Asset = Cast<UVoxelLandscapeHeightmapStampAsset>(Obj);
	if (!ensure(Asset))
	{
		return EReimportResult::Failed;
	}

	const TSharedPtr<FVoxelLandscapeHeightmapStampData> Heightmap = Import(Asset->ImportPath.FilePath);
	if (!Heightmap)
	{
		return EReimportResult::Failed;
	}

	Asset->Import(Heightmap.ToSharedRef());

	return EReimportResult::Succeeded;
}

int32 UVoxelLandscapeHeightmapStampFactory::GetPriority() const
{
	return ImportPriority;
}

TSharedPtr<FVoxelLandscapeHeightmapStampData> UVoxelLandscapeHeightmapStampFactory::Import(const FString& Path)
{
	VOXEL_FUNCTION_COUNTER();

	FString Error;
	FIntPoint Size;
	int32 BitDepth;
	TArray64<uint8> HeightmapData;
	if (!FVoxelHeightmapImporter::Import(Path, Error, Size, BitDepth, HeightmapData))
	{
		VOXEL_MESSAGE(Error, "Heightmap failed to import: {0}", Error);
		return nullptr;
	}

	TVoxelArray64<uint16> Heights;
	FVoxelUtilities::SetNumFast(Heights, Size.X * Size.Y);

	if (BitDepth == 8)
	{
		for (int32 Index = 0; Index < Size.X * Size.Y; Index++)
		{
			Heights[Index] = HeightmapData[Index] << 8;
		}
	}
	else if (BitDepth == 16)
	{
		Heights = TVoxelArray<uint16>(ReinterpretCastVoxelArrayView<uint16>(HeightmapData));
	}
	else if (BitDepth == 32)
	{
		const TVoxelArrayView64<float> FloatHeights = ReinterpretCastVoxelArrayView<float>(HeightmapData);

		float Min = FloatHeights[0];
		float Max = FloatHeights[0];
		for (const float Height : FloatHeights)
		{
			Min = FMath::Min(Min, Height);
			Max = FMath::Max(Max, Height);
		}

		for (int32 Index = 0; Index < Size.X * Size.Y; Index++)
		{
			const float Value = (FloatHeights[Index] - Min) / (Max - Min);
			Heights[Index] = FVoxelUtilities::ClampToUINT16(FMath::RoundToInt(Value));
		}
	}
	else
	{
		ensure(false);
		return nullptr;
	}

	const TSharedRef<FVoxelLandscapeHeightmapStampData> Data = MakeVoxelShared<FVoxelLandscapeHeightmapStampData>();

	Data->Initialize(
		Size.X,
		Size.Y,
		MoveTemp(Heights));

	return Data;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SVoxelLandscapeHeightmapStampFactoryDetails::Construct(const FArguments& Args)
{
	SWindow::Construct(
		SWindow::FArguments()
		.Title(INVTEXT("Import Heightmap"))
		.SizingRule(ESizingRule::Autosized));

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	const TSharedRef<IDetailsView> DetailsPanel = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsPanel->SetObject(Args._DetailsObject);

	SetContent(
		SNew(SBorder)
		.Visibility(EVisibility::Visible)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			SNew(SBox)
			.Visibility(EVisibility::Visible)
			.WidthOverride(520.0f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(500)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						DetailsPanel
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Bottom)
				.Padding(8)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(FAppStyle::GetMargin("StandardDialog.SlotPadding"))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.Text(INVTEXT("Create"))
						.HAlign(HAlign_Center)
						.IsEnabled(Args._CanCreate)
						.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
						.OnClicked_Lambda([=]
						{
							if (Args._OnCreate.Execute())
							{
								RequestDestroyWindow();
							}
							return FReply::Handled();
						})
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Success")
						.TextStyle(FAppStyle::Get(), "FlatButton.DefaultTextStyle")
					]
					+SUniformGridPanel::Slot(1,0)
					[
						SNew(SButton)
						.Text(INVTEXT("Cancel"))
						.HAlign(HAlign_Center)
						.ContentPadding(FAppStyle::GetMargin("StandardDialog.ContentPadding"))
						.OnClicked_Lambda([this]
						{
							RequestDestroyWindow();
							return FReply::Handled();
						})
						.ButtonStyle(FAppStyle::Get(), "FlatButton.Default")
						.TextStyle(FAppStyle::Get(), "FlatButton.DefaultTextStyle")
					]
				]
			]
		]);
}
