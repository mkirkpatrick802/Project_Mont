// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"

struct FVoxelNode;
class UVoxelGraphNode;
class UVoxelFunctionLibraryAsset;

struct FVoxelGraphSchemaAction : public FEdGraphSchemaAction
{
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	virtual FName GetTypeId() const final override
	{
		return StaticGetTypeId();
	}
	static FName StaticGetTypeId()
	{
		return STATIC_FNAME("FVoxelGraphSchemaAction");
	}

	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color)
	{
		static const FSlateIcon DefaultIcon("EditorStyle", "NoBrush");
		Icon = DefaultIcon;
		Color = FLinearColor::White;
	}

	UVoxelGraphNode* Apply(UEdGraph& ParentGraph, const FVector2D& Location, bool bSelectNewNode = false);
};

struct FVoxelGraphSchemaAction_NewComment : public FVoxelGraphSchemaAction
{
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_Paste : public FVoxelGraphSchemaAction
{
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

struct FVoxelGraphSchemaAction_NewCallFunctionNode : public FVoxelGraphSchemaAction
{
	FGuid Guid;
	bool bCallParent = false;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewCallExternalFunctionNode : public FVoxelGraphSchemaAction
{
	TWeakObjectPtr<UVoxelFunctionLibraryAsset> FunctionLibrary;
	FGuid Guid;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewCallGraphNode : public FVoxelGraphSchemaAction
{
	TWeakObjectPtr<UVoxelGraph> Graph;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewCallGraphParameterNode : public FVoxelGraphSchemaAction
{
	TWeakObjectPtr<UVoxelGraph> BaseGraph;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelGraphSchemaAction_NewParameterUsage : public FVoxelGraphSchemaAction
{
	FGuid Guid;
	FVoxelPinType Type;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewGraphInputUsage : public FVoxelGraphSchemaAction
{
	FGuid Guid;
	FVoxelPinType Type;
	bool bExposeDefaultAsPin = false;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewFunctionInputUsage : public FVoxelGraphSchemaAction
{
	FGuid Guid;
	FVoxelPinType Type;
	bool bExposeDefaultAsPin = false;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewOutputUsage : public FVoxelGraphSchemaAction
{
	FGuid Guid;
	FVoxelPinType Type;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewLocalVariableUsage : public FVoxelGraphSchemaAction
{
	FGuid Guid;
	FVoxelPinType Type;
	bool bIsDeclaration = false;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelGraphSchemaAction_NewParameter : public FVoxelGraphSchemaAction
{
	TOptional<FVoxelPinType> TypeOverride;
	FName NameOverride;
	FString Category;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

struct FVoxelGraphSchemaAction_NewGraphInput : public FVoxelGraphSchemaAction
{
	TOptional<FVoxelPinType> TypeOverride;
	FName NameOverride;
	FString Category;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

struct FVoxelGraphSchemaAction_NewFunctionInput : public FVoxelGraphSchemaAction
{
	TOptional<FVoxelPinType> TypeOverride;
	FName NameOverride;
	FString Category;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

struct FVoxelGraphSchemaAction_NewOutput : public FVoxelGraphSchemaAction
{
	TOptional<FVoxelPinType> TypeOverride;
	FName NameOverride;
	FString Category;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

struct FVoxelGraphSchemaAction_NewLocalVariable : public FVoxelGraphSchemaAction
{
	TOptional<FVoxelPinType> TypeOverride;
	FName NameOverride;
	FString Category;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelGraphSchemaAction_NewFunction : public FVoxelGraphSchemaAction
{
	FString Category;
	bool bOpenNewGraph = true;

	TWeakObjectPtr<UVoxelTerminalGraph> OutNewFunction;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

struct FVoxelGraphSchemaAction_NewStructNode : public FVoxelGraphSchemaAction
{
	TSharedPtr<const FVoxelNode> Node;

	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};

struct FVoxelGraphSchemaAction_NewPromotableStructNode : public FVoxelGraphSchemaAction_NewStructNode
{
	TArray<FVoxelPinType> PinTypes;

	using FVoxelGraphSchemaAction_NewStructNode::FVoxelGraphSchemaAction_NewStructNode;
	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
};

struct FVoxelGraphSchemaAction_NewKnotNode : public FVoxelGraphSchemaAction
{
	using FVoxelGraphSchemaAction::FVoxelGraphSchemaAction;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, FVector2D Location, bool bSelectNewNode = true) override;
	virtual void GetIcon(FSlateIcon& Icon, FLinearColor& Color) override;
};