// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelNode.h"
#include "VoxelGraphNode.h"
#include "VoxelTransaction.h"
#include "VoxelGraphNode_Struct.generated.h"

UCLASS()
class UVoxelGraphNode_Struct : public UVoxelGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Voxel", meta = (ShowOnlyInnerProperties))
#if CPP
	TVoxelInstancedStruct<FVoxelNode> Struct;
#else
	FVoxelInstancedStruct Struct;
#endif

	UPROPERTY()
	FString CachedName;

	//~ Begin UVoxelGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual void PrepareForCopying() override;
	virtual void PostPasteNode() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool ShowPaletteIconOnNode() const override { return true; }
	virtual bool IsCompact() const override;
	virtual bool GetOverlayInfo(FString& Type, FString& Tooltip, FString& Color) override;

	virtual bool ShowAsPromotableWildcard(const UEdGraphPin& Pin) const override;
	virtual bool IsPinOptional(const UEdGraphPin& Pin) const override;
	virtual bool ShouldHidePinDefaultValue(const UEdGraphPin& Pin) const override;
	virtual FName GetPinCategory(const UEdGraphPin& Pin) const override;

	virtual TSharedRef<IVoxelNodeDefinition> GetNodeDefinition() override;

	virtual bool CanRemovePin_ContextMenu(const UEdGraphPin& Pin) const override;
	virtual void RemovePin_ContextMenu(UEdGraphPin& Pin) override;

	virtual bool CanPromotePin(const UEdGraphPin& Pin, FVoxelPinTypeSet& OutTypes) const override;
	virtual FString GetPinPromotionWarning(const UEdGraphPin& Pin, const FVoxelPinType& NewType) const override;
	virtual void PromotePin(UEdGraphPin& Pin, const FVoxelPinType& NewType) override;

	virtual bool TryMigratePin(UEdGraphPin* OldPin, UEdGraphPin* NewPin) const override;
	virtual void TryMigrateDefaultValue(const UEdGraphPin* OldPin, UEdGraphPin* NewPin) const override;

	virtual FName GetMigratedPinName(FName PinName) const override;

	virtual void PreReconstructNode() override;

	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual bool CanJumpToDefinition() const override;
	virtual void JumpToDefinition() const override;
	//~ End UVoxelGraphNode Interface

private:
	void ReconstructFromVoxelNode();
	void InitializeDefaultValue(const FVoxelPin& VoxelPin, UEdGraphPin& GraphPin);

	friend class FVoxelNodeDefinition_Struct;
};

class FVoxelNodeDefinition_Struct : public IVoxelNodeDefinition
{
	struct FScope
	{
		FVoxelNodeDefinition_Struct* Definition;
		TUniquePtr<FVoxelTransaction> Transaction;

		explicit FScope(FVoxelNodeDefinition_Struct* Definition, const FString& Text)
			: Definition(Definition)
			, Transaction(MakeUnique<FVoxelTransaction>(&Definition->Node, Text))
		{
		}
		~FScope()
		{
			Definition->Node.ReconstructFromVoxelNode();
		}
	};

public:
	explicit FVoxelNodeDefinition_Struct(UVoxelGraphNode_Struct& Node, const TSharedRef<IVoxelNodeDefinition>& NodeDefinition) : Node(Node), NodeDefinition(NodeDefinition)
	{
	}

	virtual TSharedPtr<const FNode> GetInputs() const override;
	virtual TSharedPtr<const FNode> GetOutputs() const override;
	virtual bool OverridePinsOrder() const override { return true; }

	virtual FString GetAddPinLabel() const override;
	virtual FString GetAddPinTooltip() const override;
	virtual FString GetRemovePinTooltip() const override;

	virtual bool CanAddInputPin() const override;
	virtual void AddInputPin() override;

	virtual bool CanRemoveInputPin() const override;
	virtual void RemoveInputPin() override;

	virtual bool Variadic_CanAddPinTo(FName VariadicPinName) const override;
	virtual FName Variadic_AddPinTo(FName VariadicPinName) override;

	virtual bool Variadic_CanRemovePinFrom(FName VariadicPinName) const override;
	virtual void Variadic_RemovePinFrom(FName VariadicPinName) override;

	virtual bool CanRemoveSelectedPin(FName PinName) const override;
	virtual void RemoveSelectedPin(FName PinName) override;

	virtual void InsertPinBefore(FName PinName) override;
	virtual void DuplicatePin(FName PinName) override;

	virtual bool IsPinVisible(const UEdGraphPin* Pin, const UEdGraphNode* GraphNode) override;
	virtual bool OnPinDefaultValueChanged(FName PinName, const FVoxelPinValue& NewDefaultValue) override;

	virtual void ExposePin(FName PinName) override;

private:
	UVoxelGraphNode_Struct& Node;
	TSharedRef<IVoxelNodeDefinition> NodeDefinition;
};