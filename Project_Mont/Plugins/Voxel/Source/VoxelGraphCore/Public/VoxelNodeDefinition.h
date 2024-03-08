// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelPinValue.h"

#if WITH_EDITOR

class VOXELGRAPHCORE_API IVoxelNodeDefinition : public TSharedFromThis<IVoxelNodeDefinition>
{
public:
	class FCategoryNode;
	class FVariadicPinNode;

	class VOXELGRAPHCORE_API FNode : public TSharedFromThis<FNode>
	{
	public:
		enum class EType
		{
			Category,
			VariadicPin,
			Pin,
		};

		const EType Type;
		const FName Name;
		const TWeakPtr<FNode> WeakParent;

		virtual ~FNode() = default;

		bool IsCategory() const
		{
			return Type == EType::Category;
		}
		bool IsVariadicPin() const
		{
			return Type == EType::VariadicPin;
		}
		bool IsPin() const
		{
			return Type == EType::Pin;
		}

		const TArray<TSharedRef<FNode>>& GetChildren() const
		{
			return Children;
		}
		const TArray<FName>& GetPath() const
		{
			return PrivatePath;
		}
		FName GetConcatenatedPath() const
		{
			return PrivateConcatenatedPath;
		}
		bool IsInput() const
		{
			return bPrivateIsInput;
		}

		static TSharedRef<FCategoryNode> MakeRoot(bool bIsInput);

	protected:
		TArray<TSharedRef<FNode>> Children;
		TArray<FName> PrivatePath;
		FName PrivateConcatenatedPath;
		bool bPrivateIsInput = false;

		FNode(
			EType Type,
			FName Name,
			const TSharedPtr<FNode>& Parent);

		friend FCategoryNode;
		friend FVariadicPinNode;
	};

	class VOXELGRAPHCORE_API FVariadicPinNode : public FNode
	{
	public:
		TSharedRef<FNode> AddPin(FName PinName);

	private:
		TSet<FName> AddedPins;

		FVariadicPinNode(
			const FName Name,
			const TSharedRef<FNode>& Parent)
			: FNode(EType::VariadicPin, Name, Parent)
		{
		}

		friend class FCategoryNode;
	};

	class VOXELGRAPHCORE_API FCategoryNode : public FNode
	{
	public:
		TSharedRef<FCategoryNode> FindOrAddCategory(const FString& Category);
		TSharedRef<FCategoryNode> FindOrAddCategory(const TArray<FName>& Path);

		TSharedRef<FVariadicPinNode> AddVariadicPin(FName VariadicPinName);
		TSharedRef<FNode> AddPin(FName PinName);

	private:
		TMap<FName, TSharedPtr<FCategoryNode>> CategoryNameToNode;
		TSet<FName> AddedPins;
		TSet<FName> AddedVariadicPins;

		FCategoryNode(
			const FName Name,
			const TSharedPtr<FNode>& Parent)
			: FNode(EType::Category, Name, Parent)
		{
		}

		friend class FNode;
	};

public:
	IVoxelNodeDefinition() = default;
	virtual ~IVoxelNodeDefinition() = default;

	virtual TSharedPtr<const FNode> GetInputs() const { return nullptr; }
	virtual TSharedPtr<const FNode> GetOutputs() const { return nullptr; }
	virtual bool OverridePinsOrder() const { return false; }

	virtual FString GetAddPinLabel() const { ensure(!CanAddInputPin()); return {}; }
	virtual FString GetAddPinTooltip() const { ensure(!CanAddInputPin()); return {}; }
	virtual FString GetRemovePinTooltip() const { ensure(!CanRemoveInputPin()); return {}; }

	virtual bool CanAddInputPin() const { return false; }
	virtual void AddInputPin() VOXEL_PURE_VIRTUAL();

	virtual bool CanRemoveInputPin() const { return false; }
	virtual void RemoveInputPin() VOXEL_PURE_VIRTUAL();

	virtual bool Variadic_CanAddPinTo(FName VariadicPinName) const { return false; }
	virtual FName Variadic_AddPinTo(FName VariadicPinName) VOXEL_PURE_VIRTUAL({});

	virtual bool Variadic_CanRemovePinFrom(FName VariadicPinName) const { return false; }
	virtual void Variadic_RemovePinFrom(FName VariadicPinName) VOXEL_PURE_VIRTUAL();

	virtual bool CanRemoveSelectedPin(FName PinName) const { return false; }
	virtual void RemoveSelectedPin(FName PinName) VOXEL_PURE_VIRTUAL();

	virtual void InsertPinBefore(FName PinName) VOXEL_PURE_VIRTUAL();
	virtual void DuplicatePin(FName PinName) VOXEL_PURE_VIRTUAL();

	virtual bool IsPinVisible(const UEdGraphPin* Pin, const UEdGraphNode* Node) { return true; }
	virtual bool OnPinDefaultValueChanged(FName PinName, const FVoxelPinValue& NewDefaultValue) { return false; }

	virtual void ExposePin(FName PinName) {}
};
#endif