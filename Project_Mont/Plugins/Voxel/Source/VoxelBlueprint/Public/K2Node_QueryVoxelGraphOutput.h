// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinType.h"
#include "VoxelTerminalGraph.h"
#include "K2Node_VoxelBaseNode.h"
#include "K2Node_QueryVoxelGraphOutput.generated.h"

USTRUCT()
struct FVoxelGraphBlueprintOutput
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid Guid;

	UPROPERTY()
	FName Name;

	UPROPERTY()
	FVoxelPinType Type;

	UPROPERTY()
	bool bIsValid = true;

	FVoxelGraphBlueprintOutput() = default;
	FVoxelGraphBlueprintOutput(
		const FGuid& Guid,
		const FVoxelGraphOutput& Output)
		: Guid(Guid)
		, Name(Output.Name)
		, Type(Output.Type.GetExposedType())
	{
	}

	FString GetValue() const
	{
		return Guid.IsValid() ? Guid.ToString() : Name.ToString();
	}
};

UCLASS(Abstract)
class VOXELBLUEPRINT_API UK2Node_QueryVoxelGraphOutput : public UK2Node_VoxelBaseNode
{
	GENERATED_BODY()

public:
	UK2Node_QueryVoxelGraphOutput();

	//~ Begin UEdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	//~ End UEdGraphNode Interface

	//~ Begin UK2Node Interface
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PostReconstructNode() override;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual void PostLoad() override;
	//~ End UK2Node Interface

	virtual UEdGraphPin* GetParameterNamePin() const
	{
		return nullptr;
	}

	//~ Begin UK2Node_VoxelBaseNode Interface
	virtual void OnPinTypeChange(UEdGraphPin& Pin, const FVoxelPinType& NewType) override;
	virtual bool IsPinWildcard(const UEdGraphPin& Pin) const override;
	//~ End UK2Node_VoxelBaseNode Interface

private:
	void FixupGraphOutput();
	void SetGraphOutput(FVoxelGraphBlueprintOutput NewGraphOutput);

public:
	const static FName GraphPinName;
	const static FName OutputNamePinName;

	UPROPERTY()
	FVoxelGraphBlueprintOutput CachedGraphOutput;

	UPROPERTY()
	TObjectPtr<UVoxelGraph> CachedGraph;

private:
	FSharedVoidPtr OnOutputsChangedPtr;
};