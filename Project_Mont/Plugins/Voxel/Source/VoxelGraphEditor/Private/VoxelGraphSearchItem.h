// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "VoxelGraph.h"
#include "VoxelTerminalGraph.h"
#include "Widgets/SVoxelGraphSearch.h"

class FVoxelGraphSearchItem : public TSharedFromThis<FVoxelGraphSearchItem>
{
public:
	struct FContext
	{
		TArray<FString> Tokens;
		FVoxelGraphSearchSettings Settings;
	};

	FVoxelGraphSearchItem() = default;
	virtual ~FVoxelGraphSearchItem() = default;

	const TArray<TSharedPtr<FVoxelGraphSearchItem>>& GetChildren() const
	{
		return Children;
	}
	void Initialize(const TSharedRef<const FContext>& NewContext);

public:
	virtual EVoxelGraphSearchResultTag GetTag() const = 0;
	virtual FString GetName() const = 0;
	virtual FString GetComment() const { return {}; }
	virtual const FSlateBrush* GetIcon(FSlateColor& OutColor) const { return nullptr; }

	// Calls AddChild
	virtual void Build() {}
	virtual void OnClick();
	// Doesn't check children
	virtual bool Matches() const { return false; }

protected:
	template<typename T, typename... ArgTypes, typename = std::enable_if_t<std::is_constructible_v<T, ArgTypes...>>>
	void AddChild(ArgTypes&&... Args)
	{
		const TSharedRef<FVoxelGraphSearchItem> Child = MakeVoxelShared<T>(Forward<ArgTypes>(Args)...);
		if (!Context->Settings.ShowTag(Child->GetTag()))
		{
			return;
		}

		Child->Context = Context;
		Child->WeakParent = AsShared();
		Child->Build();

		if (MatchesString(Child->GetName()) ||
			MatchesString(Child->GetComment()) ||
			Child->Matches() ||
			Child->Children.Num() > 0)
		{
			Children.Add(Child);
		}
	}
	bool HasToken(const FString& Token) const;
	bool MatchesString(const FString& String) const;
	static TSharedPtr<FVoxelGraphToolkit> OpenToolkit(const UVoxelGraph& Graph);

private:
	TSharedPtr<const FContext> Context;
	TWeakPtr<FVoxelGraphSearchItem> WeakParent;
	TArray<TSharedPtr<FVoxelGraphSearchItem>> Children;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelGraphSearchItem_Text : public FVoxelGraphSearchItem
{
public:
	const EVoxelGraphSearchResultTag Tag;
	const FString Prefix;
	const FString Text;

	FVoxelGraphSearchItem_Text(
		const EVoxelGraphSearchResultTag Tag,
		const FString& Prefix,
		const FString& Text)
		: Tag(Tag)
		, Prefix(Prefix)
		, Text(Text)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return Tag;
	}
	virtual FString GetName() const override;
	virtual bool Matches() const override;
	//~ End FVoxelGraphSearchResult Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelGraphSearchItem_Graph : public FVoxelGraphSearchItem
{
public:
	const TWeakObjectPtr<UVoxelGraph> WeakGraph;

	explicit FVoxelGraphSearchItem_Graph(UVoxelGraph* Graph)
		: WeakGraph(Graph)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::Graph;
	}
	virtual FString GetName() const override;
	virtual const FSlateBrush* GetIcon(FSlateColor& OutColor) const override;

	virtual void Build() override;
	virtual void OnClick() override;
	//~ End FVoxelGraphSearchResult Interface
};

class FVoxelGraphSearchItem_TerminalGraph : public FVoxelGraphSearchItem
{
public:
	const TWeakObjectPtr<const UVoxelTerminalGraph> WeakTerminalGraph;

	explicit FVoxelGraphSearchItem_TerminalGraph(const UVoxelTerminalGraph& TerminalGraph)
		: WeakTerminalGraph(&TerminalGraph)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::TerminalGraph;
	}
	virtual FString GetName() const override;
	virtual const FSlateBrush* GetIcon(FSlateColor& OutColor) const override;

	virtual void Build() override;
	virtual void OnClick() override;
	//~ End FVoxelGraphSearchResult Interface
};

class FVoxelGraphSearchItem_Node : public FVoxelGraphSearchItem
{
public:
	const TWeakObjectPtr<UEdGraphNode> WeakNode;

	explicit FVoxelGraphSearchItem_Node(UEdGraphNode* Node)
		: WeakNode(Node)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::Node;
	}
	virtual FString GetName() const override;
	virtual FString GetComment() const override;
	virtual const FSlateBrush* GetIcon(FSlateColor& OutColor) const override;

	virtual void Build() override;
	virtual void OnClick() override;
	virtual bool Matches() const override;
	//~ End FVoxelGraphSearchResult Interface
};

class FVoxelGraphSearchItem_Pin : public FVoxelGraphSearchItem
{
public:
	const FEdGraphPinReference WeakPin;

	explicit FVoxelGraphSearchItem_Pin(const UEdGraphPin* Pin)
		: WeakPin(Pin)
	{

	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::Pin;
	}
	virtual FString GetName() const override;
	virtual const FSlateBrush* GetIcon(FSlateColor& OutColor) const override;

	virtual void Build() override;
	virtual bool Matches() const override;
	//~ End FVoxelGraphSearchResult Interface
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class FVoxelGraphSearchItem_Parameter : public FVoxelGraphSearchItem
{
public:
	const FGuid Guid;
	const FVoxelParameter Parameter;

	explicit FVoxelGraphSearchItem_Parameter(
		const FGuid& Guid,
		const FVoxelParameter& Parameter)
		: Guid(Guid)
		, Parameter(Parameter)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::Parameter;
	}
	virtual FString GetName() const override;
	virtual const FSlateBrush* GetIcon(FSlateColor& OutColor) const override;

	virtual void Build() override;
	virtual bool Matches() const override;
	//~ End FVoxelGraphSearchResult Interface
};

class FVoxelGraphSearchItem_Property : public FVoxelGraphSearchItem
{
public:
	const FGuid Guid;
	const FVoxelGraphProperty Property;

	FVoxelGraphSearchItem_Property(
		const FGuid& Guid,
		const FVoxelGraphProperty& Property)
		: Guid(Guid)
		, Property(Property)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual FString GetName() const override;
	virtual const FSlateBrush* GetIcon(FSlateColor& OutColor) const override;

	virtual void Build() override;
	virtual bool Matches() const override;
	//~ End FVoxelGraphSearchResult Interface
};

class FVoxelGraphSearchItem_Input : public FVoxelGraphSearchItem_Property
{
public:
	const FVoxelGraphInput Input;

	FVoxelGraphSearchItem_Input(
		const FGuid& Guid,
		const FVoxelGraphInput& Input)
		: FVoxelGraphSearchItem_Property(Guid, Input)
		, Input(Input)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::Input;
	}
	virtual void Build() override;
	//~ End FVoxelGraphSearchResult Interface
};

class FVoxelGraphSearchItem_Output : public FVoxelGraphSearchItem_Property
{
public:
	FVoxelGraphSearchItem_Output(
		const FGuid& Guid,
		const FVoxelGraphOutput& Output)
		: FVoxelGraphSearchItem_Property(Guid, Output)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::Output;
	}
	//~ End FVoxelGraphSearchResult Interface
};

class FVoxelGraphSearchItem_LocalVariable : public FVoxelGraphSearchItem_Property
{
public:
	FVoxelGraphSearchItem_LocalVariable(
		const FGuid& Guid,
		const FVoxelGraphLocalVariable& LocalVariable)
		: FVoxelGraphSearchItem_Property(Guid, LocalVariable)
	{
	}

	//~ Begin FVoxelGraphSearchResult Interface
	virtual EVoxelGraphSearchResultTag GetTag() const override
	{
		return EVoxelGraphSearchResultTag::LocalVariable;
	}
	//~ End FVoxelGraphSearchResult Interface
};