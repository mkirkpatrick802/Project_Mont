// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelEditorMinimal.h"
#include "SGraphPalette.h"

class SVoxelGraphMembers;
struct FVoxelGraphToolkit;
struct FCreateWidgetForActionData;

struct FVoxelMembersSchemaAction_Base
	: public FEdGraphSchemaAction
	, public TSharedFromThis<FVoxelMembersSchemaAction_Base>
{
public:
	FGuid MemberGuid;
	TWeakPtr<FVoxelGraphToolkit> WeakToolkit;
	TWeakPtr<SVoxelGraphMembers> WeakMembers;

public:
	using FEdGraphSchemaAction::FEdGraphSchemaAction;

	virtual FName GetTypeId() const final override;
	static FName StaticGetTypeId();

	FString GetCategory() const;

	virtual UObject* GetOuter() const = 0;
	virtual TSharedRef<SWidget> CreatePaletteWidget(const FCreateWidgetForActionData& CreateData) const = 0;
	virtual void ApplyNewGuids(const TArray<FGuid>& NewGuids) const = 0;

	virtual FEdGraphSchemaActionDefiningObject GetPersistentItemDefiningObject() const final override
	{
		return FEdGraphSchemaActionDefiningObject(GetOuter());
	}
	virtual FReply OnDragged(const FPointerEvent& MouseEvent)
	{
		return FReply::Handled();
	}
	virtual void BuildContextMenu(FMenuBuilder& MenuBuilder)
	{
	}
	virtual void OnActionDoubleClick() const
	{
	}
	virtual void OnSelected() const
	{
	}
	virtual void OnDelete() const
	{
	}
	virtual void OnDuplicate() const
	{
	}
	virtual bool OnCopy(FString& OutText) const
	{
		return false;
	}
	virtual FString GetName() const
	{
		return "";
	}
	virtual void SetName(const FString& Name) const
	{
	}
	virtual void SetCategory(const FString& NewCategory) const
	{
	}
	virtual FString GetSearchString() const
	{
		return GetName();
	}

public:
	bool IsDeleted() const
	{
		return bIsDeleted;
	}

protected:
	// Used to avoid raising ensures when querying GetName after deletion
	void MarkAsDeleted() const;

private:
	mutable bool bIsDeleted = false;

public:
	//~ Begin FEdGraphSchemaAction Interface
	virtual void MovePersistentItemToCategory(const FText& NewCategoryName) final override
	{
		SetCategory(NewCategoryName.ToString());
	}
	virtual int32 GetReorderIndexInContainer() const final override VOXEL_PURE_VIRTUAL({});
	virtual bool ReorderToBeforeAction(TSharedRef<FEdGraphSchemaAction> OtherAction) final override VOXEL_PURE_VIRTUAL({});
	//~ End FEdGraphSchemaAction Interface

protected:
	TSharedPtr<FVoxelGraphToolkit> GetToolkit() const
	{
		return WeakToolkit.Pin();
	}
	TSharedPtr<SVoxelGraphMembers> GetMembers() const
	{
		return WeakMembers.Pin();
	}

	enum class EResult
	{
		Yes,
		No,
		Cancel
	};
	static EResult CreateDeletePopups(const FString& Title, const FString& ObjectName);
};

class SVoxelMembersPaletteItem_Base : public SGraphPaletteItem
{
protected:
	TSharedRef<SWidget> CreateTextSlotWidget(const FCreateWidgetForActionData& CreateData);
	virtual void OnNameTextCommitted(const FText& NewText, ETextCommit::Type InTextCommit) override;
	virtual FText GetDisplayText() const override;
	//~ End SGraphPaletteItem Interface

	TSharedPtr<FVoxelMembersSchemaAction_Base> GetAction() const;

private:
	using SGraphPaletteItem::CreateTextSlotWidget;
};

template<typename T>
class SVoxelMembersPaletteItem : public SVoxelMembersPaletteItem_Base
{
protected:
	TSharedPtr<T> GetAction() const
	{
		return StaticCastSharedPtr<T>(SVoxelMembersPaletteItem_Base::GetAction());
	}
};