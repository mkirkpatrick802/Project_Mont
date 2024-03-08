// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNode.h"
#include "VoxelExecNode.generated.h"

struct FVoxelSpawnable;
class FVoxelRuntime;
class FVoxelExecNodeRuntime;

USTRUCT(Category = "Exec Nodes", meta = (Abstract, NodeColor = "Red", NodeIcon = "Execute", NodeIconColor = "White", ShowInRootShortList))
struct VOXELGRAPHCORE_API FVoxelExecNode : public FVoxelNode
{
	GENERATED_BODY()
	GENERATED_VOXEL_NODE_BODY()

	// If false, the node will never be executed
	VOXEL_INPUT_PIN(bool, EnableNode, true, VirtualPin);

public:
	TSharedPtr<FVoxelExecNodeRuntime> CreateSharedExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const;

protected:
	virtual TVoxelUniquePtr<FVoxelExecNodeRuntime> CreateExecRuntime(const TSharedRef<const FVoxelExecNode>& SharedThis) const VOXEL_PURE_VIRTUAL({});
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelExecNodeRuntime
	: public IVoxelNodeInterface
	, public TSharedFromThis<FVoxelExecNodeRuntime>
	, public TVoxelRuntimeInfo<FVoxelExecNodeRuntime>
{
public:
	using NodeType = FVoxelExecNode;
	const TSharedRef<const FVoxelExecNode> NodeRef;

	explicit FVoxelExecNodeRuntime(const TSharedRef<const FVoxelExecNode>& NodeRef)
		: NodeRef(NodeRef)
	{
	}
	virtual ~FVoxelExecNodeRuntime() override;

	VOXEL_COUNT_INSTANCES();

	//~ Begin IVoxelNodeInterface Interface
	FORCEINLINE virtual const FVoxelGraphNodeRef& GetNodeRef() const final override
	{
		return NodeRef->GetNodeRef();
	}
	//~ End IVoxelNodeInterface Interface

	// True before Destroy is called
	bool IsDestroyed() const
	{
		return bIsDestroyed;
	}

	const FVoxelNodeRuntime& GetNodeRuntime() const
	{
		return NodeRef->GetNodeRuntime();
	}
	TSharedRef<FVoxelTerminalGraphInstance> GetTerminalGraphInstance() const
	{
		return PrivateTerminalGraphInstance.ToSharedRef();
	}
	FORCEINLINE const FVoxelRuntimeInfo& GetRuntimeInfoRef() const
	{
		return *GetRuntimeInfo();
	}

	TSharedPtr<FVoxelRuntime> GetRuntime() const;
	USceneComponent* GetRootComponent() const;
	const TSharedRef<const FVoxelRuntimeInfo>& GetRuntimeInfo() const;

	virtual UScriptStruct* GetNodeType() const
	{
		return StaticStructFast<NodeType>();
	}

public:
	void CallCreate(
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance,
		TVoxelMap<FName, TVoxelArray<FVoxelRuntimePinValue>>&& ConstantValues);

	virtual void PreCreate() {}
	virtual void Create() {}
	virtual void Destroy() {}

	virtual void Tick(FVoxelRuntime& Runtime) {}
	virtual void AddReferencedObjects(FReferenceCollector& Collector) {}
	virtual FVoxelOptionalBox GetBounds() const { return {}; }

protected:
	FORCEINLINE const FVoxelRuntimePinValue& GetConstantPin(const FVoxelPinRef& Pin) const
	{
		const TVoxelArray<FVoxelRuntimePinValue>& Values = PrivateConstantValues[Pin];
		checkVoxelSlow(Values.Num() == 1);
		return Values[0];
	}
	FORCEINLINE const TVoxelArray<FVoxelRuntimePinValue>& GetConstantPin(const FVoxelVariadicPinRef& Pin) const
	{
		return PrivateConstantValues[Pin];
	}

	template<typename T>
	FORCEINLINE auto GetConstantPin(const TVoxelPinRef<T>& Pin) const -> decltype(auto)
	{
		const FVoxelRuntimePinValue& Value = GetConstantPin(FVoxelPinRef(Pin));

		if constexpr (VoxelPassByValue<T>)
		{
			return Value.Get<T>();
		}
		else
		{
			return Value.GetSharedStruct<T>();
		}
	}
	template<typename T>
	FORCEINLINE auto GetConstantPin(const TVoxelVariadicPinRef<T>& Pin) const -> decltype(auto)
	{
		const TVoxelArray<FVoxelRuntimePinValue>& Values = this->GetConstantPin(FVoxelVariadicPinRef(Pin));

		if constexpr (VoxelPassByValue<T>)
		{
			TVoxelArray<T> Result;
			Result.Reserve(Values.Num());
			for (const FVoxelRuntimePinValue& Value : Values)
			{
				Result.Add(Value.Get<T>());
			}
			return Result;
		}
		else
		{
			TVoxelArray<TSharedRef<const T>> Result;
			Result.Reserve(Values.Num());
			for (const FVoxelRuntimePinValue& Value : Values)
			{
				Result.Add(Value.GetSharedStruct<T>());
			}
			return Result;
		}
	}

private:
	bool bIsCreated = false;
	bool bIsDestroyed = false;
	TSharedPtr<FVoxelTerminalGraphInstance> PrivateTerminalGraphInstance;
	TVoxelMap<FName, TVoxelArray<FVoxelRuntimePinValue>> PrivateConstantValues;

	void CallDestroy();

	friend FVoxelExecNode;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename InNodeType, typename BaseType = FVoxelExecNodeRuntime>
class TVoxelExecNodeRuntime : public BaseType
{
public:
	using NodeType = InNodeType;
	using Super = TVoxelExecNodeRuntime;

	const NodeType& Node;

	explicit TVoxelExecNodeRuntime(const TSharedRef<const FVoxelExecNode>& NodeRef)
		: BaseType(NodeRef)
		, Node(CastChecked<NodeType>(*NodeRef))
	{
		checkStatic(TIsDerivedFrom<NodeType, FVoxelExecNode>::Value);
	}

	virtual UScriptStruct* GetNodeType() const override
	{
		return StaticStructFast<NodeType>();
	}
};