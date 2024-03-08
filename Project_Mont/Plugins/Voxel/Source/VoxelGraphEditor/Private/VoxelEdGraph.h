// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "Nodes/VoxelGraphNode.h"
#include "VoxelEdGraph.generated.h"

UCLASS(Deprecated)
class UDEPRECATED_VoxelGraphMacroParameterNode : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	EVoxelGraphParameterType_DEPRECATED Type;

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FVoxelGraphParameter_DEPRECATED CachedParameter;
};

UENUM()
enum class EVoxelGraphMacroType_DEPRECATED : uint8
{
	Macro,
	Template,
	RecursiveTemplate
};

UCLASS(Deprecated)
class UDEPRECATED_VoxelGraphNode_Macro : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UVoxelGraph> GraphInterface;

	UPROPERTY()
	EVoxelGraphMacroType_DEPRECATED Type = EVoxelGraphMacroType_DEPRECATED::Macro;
};

UCLASS()
class UVoxelEdGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	void SetToolkit(const TSharedRef<FVoxelGraphToolkit>& Toolkit);
	TSharedPtr<FVoxelGraphToolkit> GetGraphToolkit() const;

	void MigrateIfNeeded();
	void MigrateAndReconstructAll();

	//~ Begin UEdGraph interface
	virtual void NotifyGraphChanged(const FEdGraphEditAction& Action) override;
	virtual void PostEditUndo() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UEdGraph interface

private:
	TWeakPtr<FVoxelGraphToolkit> WeakToolkit;

private:
	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion,
		SplitInputSetterAndRemoveLocalVariablesDefault,
		AddFeatureScaleAmplitudeToBaseNoises,
		RemoveMacros,
		AddClampedLerpNodes
	);

	UPROPERTY()
	int32 Version = FVersion::FirstVersion;
};