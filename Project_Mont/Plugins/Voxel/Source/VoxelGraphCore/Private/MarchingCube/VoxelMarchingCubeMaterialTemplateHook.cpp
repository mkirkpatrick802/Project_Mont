// Copyright Voxel Plugin SAS. All Rights Reserved.

#if WITH_EDITOR
#include "VoxelMinimal.h"
#include "ShaderCore.h"
#include "UnrealEdMisc.h"
#include "Misc/ConfigCacheIni.h"
#include "HAL/PlatformFileManager.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlOperations.h"

struct FVoxelMarchingCubeMaterialTemplateHook
{
public:
	static void Apply()
	{
		VOXEL_FUNCTION_COUNTER();

		FString Path = GetShaderSourceFilePath("/Engine/Private/MaterialTemplate.ush");
		if (!ensure(!Path.IsEmpty()))
		{
			return;
		}
		Path = FPaths::ConvertRelativePathToFull(Path);

		FString MaterialTemplate;
		if (!ensure(FFileHelper::LoadFileToString(MaterialTemplate, *Path)))
		{
			return;
		}

		bool bWrite = false;
		bWrite |= ApplyVertexHooks(MaterialTemplate);
		bWrite |= ApplyPixelHooks(MaterialTemplate);

		if (!bWrite)
		{
			return;
		}

		bool bNeverApplyVoxelShaderHooks = false;
		if (GConfig->GetBool(
			TEXT("VoxelSettings"),
			TEXT("NeverApplyVoxelShaderHooks"),
			bNeverApplyVoxelShaderHooks,
			GEditorPerProjectIni))
		{
			if (bNeverApplyVoxelShaderHooks)
			{
				LOG_VOXEL(Log, "%s is out of date, but NeverApplyVoxelShaderHooks=1 - skipping", *Path);
				return;
			}
		}

		const EAppReturnType::Type DialogResult = FMessageDialog::Open(
			EAppMsgType::YesNo,
			EAppReturnType::Yes,
			FText::FromString(
				"MaterialTemplate.ush needs to be updated by Voxel Plugin.\n"
				"This is required for Voxel Detail Textures to work.\n\n"
				"Do you want to proceed with the update? This will trigger a recompile of all materials.\n\n"
				"If you're working with a source engine build, you will need to submit the following file to your source control:\n" +
				Path),
			INVTEXT("MaterialTemplate.ush is out of date"));

		if (DialogResult == EAppReturnType::No)
		{
			if (FMessageDialog::Open(
				EAppMsgType::YesNo,
				EAppReturnType::Yes,
				INVTEXT("Do you want to see this warning on the next project startup?"),
				INVTEXT("MaterialTemplate.ush is out of date")) == EAppReturnType::No)
			{
				GConfig->SetBool(TEXT("VoxelSettings"), TEXT("NeverApplyVoxelShaderHooks"), true, GEditorPerProjectIni);
			}
			return;
		}

		if (IFileManager::Get().IsReadOnly(*Path))
		{
			INLINE_LAMBDA
			{
				ISourceControlProvider & Provider = ISourceControlModule::Get().GetProvider();
				if (!Provider.IsEnabled())
				{
					VOXEL_MESSAGE(Warning, "{0} is readonly but no source control provider: manually setting it to not-readonly", Path);
					return;
				}

				const TSharedPtr<ISourceControlState> State = Provider.GetState(*Path, EStateCacheUsage::ForceUpdate);
				if (!State)
				{
					VOXEL_MESSAGE(Warning, "{0} is readonly but failed to get source control state: manually setting it to not-readonly", Path);
					return;
				}
				if (!State->IsSourceControlled())
				{
					VOXEL_MESSAGE(Warning, "{0} is readonly but file is not source controlled: manually setting it to not-readonly", Path);
					return;
				}

				if (State->IsCheckedOut())
				{
					return;
				}

				if (!State->CanCheckout())
				{
					VOXEL_MESSAGE(Warning, "{0} is readonly but file cannot be checked out: manually setting it to not-readonly", Path);
					return;
				}

				TArray<FString> FilesToBeCheckedOut;
				FilesToBeCheckedOut.Add(Path);
				if (Provider.Execute(ISourceControlOperation::Create<FCheckOut>(), FilesToBeCheckedOut) == ECommandResult::Succeeded)
				{
					VOXEL_MESSAGE(Warning, "{0} is readonly but file failed to be checked out: manually setting it to not-readonly", Path);
					return;
				}

				VOXEL_MESSAGE(Info, "{0} checked out", Path);

				if (IFileManager::Get().IsReadOnly(*Path))
				{
					VOXEL_MESSAGE(Warning, "{0} is readonly after check out: manually setting it to not-readonly", Path);
					return;
				}

				ensure(!IFileManager::Get().IsReadOnly(*Path));
			};
		}

		if (IFileManager::Get().IsReadOnly(*Path))
		{
			if (!ensure(FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Path, false)))
			{
				FMessageDialog::Open(
					EAppMsgType::Ok,
					EAppReturnType::Ok,
					FText::FromString("Failed to clear readonly flag on " + Path),
					INVTEXT("MaterialTemplate.ush is out of date"));
			}
		}

		if (!FFileHelper::SaveStringToFile(MaterialTemplate, *Path))
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				EAppReturnType::Ok,
				FText::FromString("Failed to write " + Path),
				INVTEXT("MaterialTemplate.ush is out of date"));
		}

		FMessageDialog::Open(
			EAppMsgType::Ok,
			EAppReturnType::Ok,
			FText::FromString(Path + " successfully updated\n\nThe engine will now restart"),
			INVTEXT("MaterialTemplate.ush is out of date"));

		FUnrealEdMisc::Get().RestartEditor(false);
	}

private:
	static bool Parse(const FString& MaterialTemplate, int32& Index, const FString& Text)
	{
		int32 LocalIndex = Index;
		int32 TextIndex = 0;
		while (TextIndex < Text.Len())
		{
			if (!ensure(LocalIndex < MaterialTemplate.Len()))
			{
				return false;
			}
			if (MaterialTemplate[LocalIndex] == Text[TextIndex])
			{
				LocalIndex++;
				TextIndex++;
				continue;
			}

			if (FChar::IsWhitespace(MaterialTemplate[LocalIndex]))
			{
				LocalIndex++;
				continue;
			}

			return false;
		}

		Index = LocalIndex;
		ensure(TextIndex == Text.Len());
		return true;
	}
	static bool ApplyVertexHooks(FString& MaterialTemplate)
	{
		VOXEL_FUNCTION_COUNTER();

		TArray<FString> LinesToAdd =
		{
			"#if VOXEL_MARCHING_CUBE_VERTEX_FACTORY",
			"	uint VoxelDetailTexture_CellIndex;",
			"	float2 VoxelDetailTexture_Delta;",
			"	float3 VoxelDetailTexture_MaterialIds;",
			"	float2 VoxelDetailTexture_LerpAlphas;",
			"#define VERTEX_PARAMETERS_HAS_VoxelDetailTexture_CellIndex 1",
			"#define VERTEX_PARAMETERS_HAS_VoxelDetailTexture_Delta 1",
			"#define VERTEX_PARAMETERS_HAS_VoxelDetailTexture_MaterialIds_LerpAlphas 1",
			"#endif"
		};

		int32 Index = MaterialTemplate.Find(TEXT("#if WATER_MESH_FACTORY"), ESearchCase::CaseSensitive);
		if (!ensure(Index != -1))
		{
			return false;
		}

		// Find second one, first one is pixel
		Index = MaterialTemplate.Find(TEXT("#if WATER_MESH_FACTORY"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index + 1);
		if (!ensure(Index != -1))
		{
			return false;
		}

		if (!ensure(Parse(MaterialTemplate, Index, "#if WATER_MESH_FACTORY"))) return false;
		if (!ensure(Parse(MaterialTemplate, Index, "uint WaterWaveParamIndex;"))) return false;
		if (!ensure(Parse(MaterialTemplate, Index, "#endif"))) return false;

		const int32 InsertIndex = Index;

		if (Parse(MaterialTemplate, Index, "// BEGIN VOXEL SHADER"))
		{
			const FString EndVoxelShader = "// END VOXEL SHADER";
			const int32 EndIndex = MaterialTemplate.Find(
				EndVoxelShader,
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				Index);
			if (!ensure(EndIndex != -1))
			{
				return false;
			}

			const FString VoxelText = MaterialTemplate.Mid(Index, EndIndex - Index);

			if (!ensure(!VoxelText.Contains(TEXT("}"))))
			{
				return false;
			}

			TArray<FString> ParsedLines;
			VoxelText.ParseIntoArrayLines(ParsedLines, false);

			TArray<FString> Lines;
			for (FString& Line : ParsedLines)
			{
				Line.TrimStartAndEndInline();
				if (Line.IsEmpty())
				{
					continue;
				}

				if (!Line.StartsWith("#"))
				{
					Line = TEXT("\t") + Line;
				}
				Lines.Add(Line);
			}

			if (Lines == LinesToAdd)
			{
				return false;
			}

			int32 Count = EndIndex - InsertIndex + EndVoxelShader.Len();
			if (MaterialTemplate[InsertIndex + Count] == TEXT('\r'))
			{
				Count++;
			}
			if (MaterialTemplate[InsertIndex + Count] == TEXT('\n'))
			{
				Count++;
			}

			MaterialTemplate.RemoveAt(InsertIndex, Count);
		}

		Index = InsertIndex;

		if (!ensure(Parse(MaterialTemplate, Index, "FMaterialAttributes MaterialAttributes;")))
		{
			return false;
		}

		LinesToAdd.Insert("", 0);
		LinesToAdd.Insert("// BEGIN VOXEL SHADER", 1);
		LinesToAdd.Add("// END VOXEL SHADER");
		LinesToAdd.Add("");

		FString MergedLines;
		if (MaterialTemplate.Contains(TEXT("\r\n")))
		{
			MergedLines = FString::Join(LinesToAdd, TEXT("\r\n"));
		}
		else
		{
			MergedLines = FString::Join(LinesToAdd, TEXT("\n"));
		}

		MaterialTemplate.InsertAt(InsertIndex, MergedLines);
		return true;
	}
	static bool ApplyPixelHooks(FString& MaterialTemplate)
	{
		VOXEL_FUNCTION_COUNTER();

		TArray<FString> LinesToAdd =
		{
			"#if VOXEL_MARCHING_CUBE_VERTEX_FACTORY",
			"	float3 VoxelDetailTexture_DebugColor;",
			"	uint VoxelDetailTexture_CellIndex;",
			"	float2 VoxelDetailTexture_Delta;",
			"	float3 VoxelDetailTexture_MaterialIds;",
			"	float2 VoxelDetailTexture_LerpAlphas;",
			"#define PIXEL_PARAMETERS_HAS_VoxelDetailTexture_DebugColor 1",
			"#define PIXEL_PARAMETERS_HAS_VoxelDetailTexture_CellIndex 1",
			"#define PIXEL_PARAMETERS_HAS_VoxelDetailTexture_Delta 1",
			"#define PIXEL_PARAMETERS_HAS_VoxelDetailTexture_MaterialIds_LerpAlphas 1",
			"#endif"
		};

		int32 Index = MaterialTemplate.Find(TEXT("#if WATER_MESH_FACTORY"), ESearchCase::CaseSensitive);
		if (!ensure(Index != -1))
		{
			return false;
		}

		if (!ensure(Parse(MaterialTemplate, Index, "#if WATER_MESH_FACTORY"))) return false;
		if (!ensure(Parse(MaterialTemplate, Index, "uint WaterWaveParamIndex;"))) return false;
		if (!ensure(Parse(MaterialTemplate, Index, "#endif"))) return false;

		const int32 InsertIndex = Index;

		if (Parse(MaterialTemplate, Index, "// BEGIN VOXEL SHADER"))
		{
			const FString EndVoxelShader = "// END VOXEL SHADER";
			const int32 EndIndex = MaterialTemplate.Find(
				EndVoxelShader,
				ESearchCase::CaseSensitive,
				ESearchDir::FromStart,
				Index);
			if (!ensure(EndIndex != -1))
			{
				return false;
			}

			const FString VoxelText = MaterialTemplate.Mid(Index, EndIndex - Index);

			if (!ensure(!VoxelText.Contains(TEXT("}"))))
			{
				return false;
			}

			TArray<FString> ParsedLines;
			VoxelText.ParseIntoArrayLines(ParsedLines, false);

			TArray<FString> Lines;
			for (FString& Line : ParsedLines)
			{
				Line.TrimStartAndEndInline();
				if (Line.IsEmpty())
				{
					continue;
				}

				if (!Line.StartsWith("#"))
				{
					Line = TEXT("\t") + Line;
				}
				Lines.Add(Line);
			}

			if (Lines == LinesToAdd)
			{
				return false;
			}

			int32 Count = EndIndex - InsertIndex + EndVoxelShader.Len();
			if (MaterialTemplate[InsertIndex + Count] == TEXT('\r'))
			{
				Count++;
			}
			if (MaterialTemplate[InsertIndex + Count] == TEXT('\n'))
			{
				Count++;
			}

			MaterialTemplate.RemoveAt(InsertIndex, Count);
		}

		Index = InsertIndex;

		if (!ensure(Parse(MaterialTemplate, Index, "#if CLOUD_LAYER_PIXEL_SHADER")))
		{
			return false;
		}

		LinesToAdd.Insert("", 0);
		LinesToAdd.Insert("// BEGIN VOXEL SHADER", 1);
		LinesToAdd.Add("// END VOXEL SHADER");
		LinesToAdd.Add("");

		FString MergedLines;
		if (MaterialTemplate.Contains(TEXT("\r\n")))
		{
			MergedLines = FString::Join(LinesToAdd, TEXT("\r\n"));
		}
		else
		{
			MergedLines = FString::Join(LinesToAdd, TEXT("\n"));
		}

		MaterialTemplate.InsertAt(InsertIndex, MergedLines);
		return true;
	}
};

VOXEL_RUN_ON_STARTUP_EDITOR(ApplyVoxelMarchingCubeMaterialTemplateHook)
{
	FVoxelUtilities::DelayedCall([]
	{
		FVoxelMarchingCubeMaterialTemplateHook::Apply();
	}, 1.f);
}
#endif