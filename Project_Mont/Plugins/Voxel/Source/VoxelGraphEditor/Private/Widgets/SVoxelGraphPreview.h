// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelTransformRef.h"
#include "VoxelGraphNodeRef.h"

class UVoxelTerminalGraph;
class SVoxelGraphPreviewImage;
class SVoxelGraphPreviewScale;
class SVoxelGraphPreviewRuler;
class SVoxelGraphPreviewStats;
class SVoxelGraphPreviewDepthSlider;
struct FVoxelPreviewHandler;

class SVoxelGraphPreview : public SCompoundWidget
{
public:
	VOXEL_SLATE_ARGS()
	{
	};

	void Construct(const FArguments& Args);

	void QueueUpdate()
	{
		bUpdateQueued = true;
	}
	void SetTerminalGraph(UVoxelTerminalGraph* NewTerminalGraph);

	TSharedRef<SWidget> GetPreviewStats() const;
	void AddReferencedObjects(FReferenceCollector& Collector);

	//~ Begin SWidget Interface
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	//~ End SWidget Interface

private:
	TObjectPtr<USceneComponent> SceneComponent;
	FVoxelTransformRef TransformRef;

	TWeakObjectPtr<UVoxelTerminalGraph> WeakTerminalGraph;
	FSharedVoidPtr OnEdGraphChangedPtr;

	bool bUpdateQueued = false;
	double ProcessingStartTime = 0;
	FString Message;

	TSharedPtr<FVoxelPreviewHandler> PreviewHandler;

	TSharedPtr<SVoxelGraphPreviewStats> PreviewStats;
	TSharedPtr<SVoxelGraphPreviewImage> PreviewImage;
	TSharedPtr<SVoxelGraphPreviewRuler> PreviewRuler;

	TSharedPtr<SBox> DepthSliderContainer;
	TSharedPtr<SVoxelGraphPreviewDepthSlider> DepthSlider;

	FVector2D MousePosition = FVector2D::ZeroVector;

	bool bLockCoordinatePending = false;
	bool bIsCoordinateLocked = false;
	FVector LockedCoordinate_WorldSpace = FVector::ZeroVector;

	void Update();
	void UpdateStats();
	FMatrix GetPixelToWorld() const;
};