// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelVoxelizedMeshAssetToolkit.h"
#include "VoxelActor.h"
#include "VoxelGraph.h"
#include "VoxelInvoker.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshData.h"
#include "SceneManagement.h"

void FVoxelVoxelizedMeshAssetToolkit::Tick()
{
	VOXEL_FUNCTION_COUNTER();

	Super::Tick();

	Actor->SetParameterChecked("Mesh", Asset);
	Actor->SetParameterChecked("VoxelSize", Asset->VoxelSize);

	const FNumberFormattingOptions Options = FNumberFormattingOptions().SetMinimumFractionalDigits(2).SetMaximumFractionalDigits(2);

	const TSharedPtr<const FVoxelVoxelizedMeshData> Data = Asset->GetData();
	if (!Data)
	{
		return;
	}

	const double SizeInMB = Data->GetAllocatedSize() / double(1 << 20);

	UpdateStatsText(FString() +
		"<TextBlock.ShadowedText>Memory size: </>" +
		"<TextBlock.ShadowedTextWarning>" + FText::AsNumber(SizeInMB, &Options).ToString() + "MB</>\n" +
		"<TextBlock.ShadowedText>Data size: " + Data->Size.ToString() + "</>");
}

void FVoxelVoxelizedMeshAssetToolkit::SetupPreview()
{
	VOXEL_FUNCTION_COUNTER();

	Super::SetupPreview();

	Actor = SpawnActor<AVoxelActor>();
	if (!ensure(Actor))
	{
		return;
	}

	UVoxelInvokerComponent* Component = CreateComponent<UVoxelInvokerComponent>();
	if (!ensure(Component))
	{
		return;
	}
	Component->Radius = 10000;

	UVoxelGraph* Graph = LoadObject<UVoxelGraph>(nullptr, TEXT("/Script/VoxelGraphCore.VoxelGraph'/Voxel/Default/VG_RenderBrush.VG_RenderBrush'"));
	if (!ensure(Graph))
	{
		return;
	}

	Actor->SetGraph(Graph);
}

void FVoxelVoxelizedMeshAssetToolkit::DrawPreview(const FSceneView* View, FPrimitiveDrawInterface* PDI)
{
	Super::DrawPreview(View, PDI);

	if (!ensure(Asset) ||
		!Asset->Mesh ||
		Asset->Mesh->IsCompiling())
	{
		return;
	}

	const TSharedPtr<const FVoxelVoxelizedMeshData> Data = Asset->GetData();
	if (!Data)
	{
		return;
	}

	const FBox Box =
		FVoxelBox(0, FVector(Data->Size))
		.ShiftBy(FVector(Data->Origin))
		.Scale(Data->VoxelSize)
		.ToFBox();

	DrawWireBox(PDI, Box, FLinearColor(0.0f, 0.48828125f, 0.0f), SDPG_World);
}

TOptional<float> FVoxelVoxelizedMeshAssetToolkit::GetInitialViewDistance() const
{
	const TSharedPtr<const FVoxelVoxelizedMeshData> Data = Asset->GetData();
	if (!Data)
	{
		return {};
	}

	return Data->Size.Size() * Asset->VoxelSize;
}