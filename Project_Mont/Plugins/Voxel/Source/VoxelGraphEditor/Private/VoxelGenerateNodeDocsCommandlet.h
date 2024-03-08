// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "Commandlets/Commandlet.h"
#include "VoxelGenerateNodeDocsCommandlet.generated.h"

UCLASS()
class UVoxelGenerateNodeDocsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	//~ Begin UCommandlet Interface
	virtual int32 Main(const FString& Params) override;
	//~ End UCommandlet Interface

private:
	static FString RunCmd(
		const FString& CommandLine,
		bool bAllowFailure = false);

	static FString RunCmd(
		const FString& CommandLine,
		const FString& WorkingDirectory,
		bool bAllowFailure = false,
		TFunction<void()> OnFailure = nullptr);

	static FString GetPluginDocPath();

private:
	void Generate(bool bIsGatheringTexture);

	struct FNode
	{
		FString DisplayName;
		FString Url;
		TMap<FString, TSharedPtr<FNode>> Children;
	};
	TSharedPtr<FNode> RootNode;
	TMap<FString, TArray64<uint8>> PreviousFiles;
};