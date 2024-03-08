// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelGenerateNodeDocsCommandlet.h"
#include "VoxelEdGraph.h"
#include "VoxelNodeLibrary.h"
#include "VoxelGraphSchema.h"
#include "Nodes/VoxelGraphNode_Struct.h"

#include "ImageUtils.h"
#include "NodeFactory.h"
#include "SGraphPanel.h"
#include "TextureResource.h"
#include "Slate/WidgetRenderer.h"
#include "Internationalization/Regex.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Interfaces/ISlateRHIRendererModule.h"

int32 UVoxelGenerateNodeDocsCommandlet::Main(const FString& Params)
{
	if (!FPaths::DirectoryExists(GetPluginDocPath()))
	{
		RunCmd("git clone git@github.com:VoxelPluginDev/VoxelPluginDocs", FString("C:/ROOT/Temp"));
	}

	RunCmd("git reset --hard");
	RunCmd("git pull");

	const FString APIDirectory = GetPluginDocPath() / "api";
	{
		IFileManager::Get().IterateDirectoryRecursively(*APIDirectory, [&](const TCHAR* RawFilename, const bool bIsDirectory)
		{
			if (bIsDirectory)
			{
				return true;
			}
			const FString Filename(RawFilename);

			TArray64<uint8> Data;
			verify(FFileHelper::LoadFileToArray(Data, *Filename));
			PreviousFiles.Add(Filename, Data);

			return true;
		});
	}
	verify(IFileManager::Get().DeleteDirectory(*APIDirectory, false, true));
	verify(IFileManager::Get().MakeDirectory(*APIDirectory, true));

	const FString SummaryPath = GetPluginDocPath() / "SUMMARY.md";
	{
		FString Summary;
		verify(FFileHelper::LoadFileToString(Summary, *SummaryPath));
		Summary.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

		verify(Summary.RemoveFromStart("# Table of contents\n\n"));

		TArray<FString> Lines;
		Summary.ParseIntoArray(Lines, TEXT("\n"));

		RootNode = MakeVoxelShared<FNode>();

		TArray<TSharedPtr<FNode>> Nodes;
		Nodes.Add(RootNode);

		for (FString& Line : Lines)
		{
			int32 LineDepth = 1;
			while (Line.RemoveFromStart("  "))
			{
				LineDepth++;
			}
			verify(Line.RemoveFromStart("* "));

			const FString Pattern = "\\[(.*)\\]\\((.*)\\)";
			FRegexPattern RegexPattern(Pattern);
			FRegexMatcher RegexMatcher(RegexPattern, Line);
			verify(RegexMatcher.FindNext());

			const TSharedRef<FNode> Node = MakeVoxelShared<FNode>();
			Node->DisplayName = RegexMatcher.GetCaptureGroup(1);
			Node->Url = RegexMatcher.GetCaptureGroup(2);
			Node->Url.RemoveFromStart("<");
			Node->Url.RemoveFromEnd(">");

			while (LineDepth < Nodes.Num())
			{
				Nodes.Pop(false);
			}
			verify(LineDepth == Nodes.Num());

			verify(!Nodes.Last()->Children.Contains(Node->DisplayName));
			Nodes.Last()->Children.Add(Node->DisplayName, Node);

			Nodes.Add(Node);
		}
	}

	RootNode->Children["API"]->Children.Empty();

	verify(FParse::Param(FCommandLine::Get(), TEXT("AllowCommandletRendering")));

	FSlateApplication::InitHighDPI(true);
	FSlateApplication::Create();

	const TSharedRef<FSlateRenderer> SlateRenderer = FModuleManager::Get().GetModuleChecked<ISlateRHIRendererModule>("SlateRHIRenderer").CreateSlateRHIRenderer();
	FSlateApplication::Get().InitializeRenderer(SlateRenderer);

	Generate(true);

	// Update texture atlases
	// Do a dummy render first to force vector brushes to cache
	{
		FSlateRenderer::FScopedAcquireDrawBuffer ScopedDrawBuffer{ *SlateRenderer };
		SlateRenderer->DrawWindows(ScopedDrawBuffer.GetDrawBuffer());
	}

	Generate(false);

	{
		const TFunction<void(const FNode&)> Traverse = [&](const FNode& Node)
		{
			const FString Path = GetPluginDocPath() / Node.Url;

			FString Text = "# " + Node.DisplayName + "\n\n";
			for (const auto& It : Node.Children)
			{
				// TODO proper link
				// Text += "[" + It.Key + "](<" + It.Value->Url + ">)\n\n";
				Traverse(*It.Value);
			}

			if (Node.Children.Num() == 0)
			{
				verify(IFileManager::Get().FileExists(*Path));
			}
			else
			{
				verify(!IFileManager::Get().FileExists(*Path));
				verify(FFileHelper::SaveStringToFile(Text, *Path));
			}
		};
		Traverse(*RootNode->Children["API"]);
	}

	{
		FString Summary = "# Table of contents\n\n";

		const TFunction<void(const FNode&, int32)> Traverse = [&](const FNode& Node, const int32 Depth)
		{
			for (int32 Index = 0; Index < Depth; Index++)
			{
				Summary += "  ";
			}
			Summary += "* [" + Node.DisplayName + "](<" + Node.Url + ">)\n";

			for (const auto& It : Node.Children)
			{
				Traverse(*It.Value, Depth + 1);
			}
		};

		for (const auto& It : RootNode->Children)
		{
			Traverse(*It.Value, 0);
		}

		verify(FFileHelper::SaveStringToFile(Summary, *SummaryPath));
		LOG_VOXEL(Display, "Saved %s", *SummaryPath);
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

FString UVoxelGenerateNodeDocsCommandlet::RunCmd(
	const FString& CommandLine,
	const bool bAllowFailure)
{
	return RunCmd(CommandLine, GetPluginDocPath(), bAllowFailure);
}

FString UVoxelGenerateNodeDocsCommandlet::RunCmd(
	const FString& CommandLine,
	const FString& WorkingDirectory,
	const bool bAllowFailure,
	const TFunction<void()> OnFailure)
{
	UE_LOG(LogTemp, Display, TEXT("Running %s"), *CommandLine);

	FString Executable;
	FString FinalCommandLine;
	if (CommandLine.StartsWith("git "))
	{
		Executable = PLATFORM_WINDOWS ? "git.exe" : "/opt/homebrew/bin/git";
		FinalCommandLine = CommandLine;
		FinalCommandLine.RemoveFromStart("git ");
	}
	else
	{
		if (PLATFORM_WINDOWS)
		{
			Executable = "cmd.exe";
			FinalCommandLine = "/c \"" + CommandLine + "\"";
		}
		else
		{
			verify(FFileHelper::SaveStringToFile(CommandLine, TEXT("/Volumes/EXTERNAL/Temp/script.sh")));

			Executable = "/bin/bash";
			FinalCommandLine = "/Volumes/EXTERNAL/Temp/script.sh";
		}
	}

	void* PipeRead = nullptr;
	void* PipeWrite = nullptr;
	verify(FPlatformProcess::CreatePipe(PipeRead, PipeWrite));

	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*Executable,
		*FinalCommandLine,
		true,
		false,
		false,
		nullptr,
		0,
		*WorkingDirectory,
		PipeWrite,
		nullptr,
		PipeWrite);

	verify(ProcHandle.IsValid());

	FString Output;
	while (FPlatformProcess::IsProcRunning(ProcHandle))
	{
		const FString Read = FPlatformProcess::ReadPipe(PipeRead);
		if (!Read.IsEmpty())
		{
			Output += Read;
			UE_LOG(LogTemp, Log, TEXT("%s"), *Read);
		}
		FPlatformProcess::Sleep(0.1f);
	}

	int32 ReturnCode = 1;
	verify(FPlatformProcess::GetProcReturnCode(ProcHandle, &ReturnCode));

	while (true)
	{
		const FString Read = FPlatformProcess::ReadPipe(PipeRead);
		if (Read.IsEmpty())
		{
			break;
		}

		Output += Read;
		UE_LOG(LogTemp, Log, TEXT("%s"), *Read);
	}
	FPlatformProcess::ClosePipe(PipeRead, PipeWrite);

	if (ReturnCode != 0)
	{
		if (OnFailure)
		{
			OnFailure();
		}

		if (!bAllowFailure)
		{
			UE_LOG(LogTemp, Fatal, TEXT("%s failed"), *CommandLine);
		}
	}

	Output.RemoveFromEnd("\n");
	return Output;
}

FString UVoxelGenerateNodeDocsCommandlet::GetPluginDocPath()
{
	return "C:/ROOT/Temp/VoxelPluginDocs";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelGenerateNodeDocsCommandlet::Generate(const bool bIsGatheringTexture)
{
	VOXEL_FUNCTION_COUNTER();

	const FVector2D DrawSize(4096.f, 4096.f);

	UTextureRenderTarget2D* RenderTarget = FWidgetRenderer::CreateTargetFor(DrawSize, TF_Bilinear, false);
	verify(RenderTarget);

	UVoxelEdGraph* Graph = NewObject<UVoxelEdGraph>();
	Graph->Schema = UVoxelGraphSchema::StaticClass();

	const TSharedRef<SGraphPanel> GraphPanel = SNew(SGraphPanel).GraphObj(Graph);

	// High zoom for highest LOD
	GraphPanel->RestoreViewSettings(FVector2D(0, 0), 10.0f);

	for (const TSharedRef<const FVoxelNode>& Node : FVoxelNodeLibrary::GetNodes())
	{
		UVoxelGraphNode_Struct* StructNode = NewObject<UVoxelGraphNode_Struct>(Graph);
		StructNode->Struct = *Node;
		StructNode->bShowAdvanced = true;
		StructNode->AllocateDefaultPins();

		const TSharedPtr<SGraphNode> Widget = FNodeFactory::CreateNodeWidget(StructNode);
		verify(Widget);

		const TSharedRef<SBorder> WrappedWidget =
			SNew(SBorder)
			.ForegroundColor(FColor::White)
			[
				Widget.ToSharedRef()
			];

		constexpr int32 Scale = 1;

		FWidgetRenderer WidgetRenderer;
		WidgetRenderer.SetUseGammaCorrection(true);
		WidgetRenderer.DrawWidget(RenderTarget, WrappedWidget, Scale, DrawSize, 0.f);

		const FVector2f DesiredSize = WrappedWidget->GetDesiredSize();
		const int32 SizeX = FMath::CeilToInt(DesiredSize.X) * Scale;
		const int32 SizeY = FMath::CeilToInt(DesiredSize.Y) * Scale;

		FTextureRenderTargetResource* Resource = RenderTarget->GameThread_GetRenderTargetResource();
		verify(Resource);

		const FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);

		TArray<FColor> ImageData;
		verify(Resource->ReadPixels(ImageData, ReadPixelFlags, FIntRect(0, 0, SizeX, SizeY)));
		verify(ImageData.Num() == SizeX * SizeY);

		if (bIsGatheringTexture)
		{
			continue;
		}

		const FString DisplayName = Node->GetDisplayName();
		const FString CategoryString = Node->GetCategory();

		FString SanitizedDisplayName = DisplayName;
		SanitizedDisplayName.ReplaceInline(TEXT(" "), TEXT("_"));
		SanitizedDisplayName.ReplaceInline(TEXT("%"), TEXT("_"));

		TArray<FString> Categories;
		CategoryString.ParseIntoArray(Categories, TEXT("|"));

		FString BasePath = GetPluginDocPath() / "api";
		for (const FString& Category : Categories)
		{
			BasePath /= Category;
		}
		verify(IFileManager::Get().MakeDirectory(*BasePath, true));

		TSharedPtr<FNode> SummaryNode = RootNode->Children["API"];
		FString SummaryPath = "api";
		{
			for (const FString& Category : Categories)
			{
				SummaryPath /= Category;

				TSharedPtr<FNode>& ChildNode = SummaryNode->Children.FindOrAdd(Category);
				if (!ChildNode)
				{
					ChildNode = MakeVoxelShared<FNode>();
					ChildNode->DisplayName = Category;
					ChildNode->Url = SummaryPath / "README.md";
				}
				SummaryNode = ChildNode;
			}
		}

		{
			const TSharedRef<FNode> ChildNode = MakeVoxelShared<FNode>();
			ChildNode->DisplayName = DisplayName;
			ChildNode->Url = SummaryPath / SanitizedDisplayName + ".md";

			verify(!SummaryNode->Children.Contains(DisplayName));
			SummaryNode->Children.Add(DisplayName, ChildNode);
		}

		const FString ImagePath = BasePath / SanitizedDisplayName + ".png";
		const FString PagePath = BasePath / SanitizedDisplayName + ".md";

		FImageView Image(ImageData.GetData(), SizeX, SizeY);
		FImage PreviousImage;

		INLINE_LAMBDA
		{
			const TArray64<uint8>* PreviousFile = PreviousFiles.Find(ImagePath);
			if (!PreviousFile)
			{
				return;
			}

			verify(FImageUtils::DecompressImage(PreviousFile->GetData(), PreviousFile->Num(), PreviousImage));

			const TArrayView64<FColor> PreviousColors = PreviousImage.AsBGRA8();
			const TArrayView64<FColor> NewColors = Image.AsBGRA8();
			if (PreviousColors.Num() != NewColors.Num())
			{
				return;
			}

			double Error = 0.;
			for (int32 Index = 0; Index < PreviousColors.Num(); Index++)
			{
				const FLinearColor PreviousColor = FLinearColor(PreviousColors[Index]);
				const FLinearColor NewColor = FLinearColor(NewColors[Index]);

				Error += FMath::Square(NewColor.R - PreviousColor.R);
				Error += FMath::Square(NewColor.G - PreviousColor.G);
				Error += FMath::Square(NewColor.B - PreviousColor.B);
				Error += FMath::Square(NewColor.A - PreviousColor.A);
			}

			Error /= 4;
			Error /= PreviousColors.Num();

			if (Error > 1.e-9)
			{
				return;
			}

			Image = PreviousImage;
		};

		TArray64<uint8> ScaledPng;
		verify(FImageUtils::CompressImage(ScaledPng, *ImagePath, Image));
		verify(FFileHelper::SaveArrayToFile(ScaledPng, *ImagePath));

		LOG_VOXEL(Display, "Saved %s", *ImagePath);

		FString PageText = R"(
# NODE_TITLE

<div align="left" data-full-width="false">

<figure><img src="SANITIZED_DISPLAY_NAME.png" alt=""><figcaption></figcaption></figure>

</div>

NODE_DESCRIPTION

## Inputs

<table>
<thead><tr><th width="170">Name</th><th>Description</th></tr></thead>
<tbody>NODE_INPUT_ROWS
</tbody>
</table>

## Outputs

<table>
<thead><tr><th width="170">Name</th><th>Description</th></tr></thead>
<tbody>NODE_OUTPUT_ROWS
</tbody>
</table>
)";

		PageText.TrimStartAndEndInline();
		PageText.ReplaceInline(TEXT("NODE_TITLE"), *DisplayName, ESearchCase::CaseSensitive);
		PageText.ReplaceInline(TEXT("SANITIZED_DISPLAY_NAME"), *SanitizedDisplayName, ESearchCase::CaseSensitive);
		PageText.ReplaceInline(TEXT("NODE_DESCRIPTION"), *Node->GetTooltip(), ESearchCase::CaseSensitive);

		FString InputNodeRows;
		FString OutputNodeRows;
		for (const FVoxelPin& Pin : Node->GetPins())
		{
			(Pin.bIsInput ? InputNodeRows : OutputNodeRows) += FString("\n<tr>") +
				"<td>" + FName::NameToDisplayString(Pin.Name.ToString(), false) + "</td>" +
				"<td>" + Pin.Metadata.Tooltip.Get() + "</td>" +
				"</tr>";
		}
		PageText.ReplaceInline(TEXT("NODE_INPUT_ROWS"), *InputNodeRows, ESearchCase::CaseSensitive);
		PageText.ReplaceInline(TEXT("NODE_OUTPUT_ROWS"), *OutputNodeRows, ESearchCase::CaseSensitive);

		verify(FFileHelper::SaveStringToFile(PageText, *PagePath));
		LOG_VOXEL(Display, "Saved %s", *PagePath);
	}
}