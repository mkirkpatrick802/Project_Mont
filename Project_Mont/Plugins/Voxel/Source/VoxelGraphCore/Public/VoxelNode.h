// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelNodeHelpers.h"
#include "VoxelNodeInterface.h"
#include "VoxelNodeDefinition.h"
#include "Misc/Attribute.h"
#include "VoxelNode.generated.h"

class UVoxelGraph;
class FVoxelNodeDefinition;

enum class EVoxelPinFlags : uint32
{
	None         = 0,
	TemplatePin  = 1 << 0,
	VariadicPin  = 1 << 1,
};
ENUM_CLASS_FLAGS(EVoxelPinFlags);

struct FVoxelPinMetadataFlags
{
	bool bArrayPin = false;
	bool bVirtualPin = false;
	bool bConstantPin = false;
	bool bOptionalPin = false;
	bool bDisplayLast = false;
	bool bNoDefault = false;
	bool bShowInDetail = false;
	bool bHidePin = false;
};

struct FVoxelPinMetadata : FVoxelPinMetadataFlags
{
#if WITH_EDITOR
	FString DisplayName;
	FString Category;
	TAttribute<FString> Tooltip;
	FString DefaultValue;
	int32 Line = 0;
	UScriptStruct* Struct = nullptr;
#endif

	UClass* BaseClass = nullptr;
};

struct VOXELGRAPHCORE_API FVoxelPin
{
public:
	const FName Name;
	const bool bIsInput;
	const float SortOrder;
	const FName VariadicPinName;
	const EVoxelPinFlags Flags;
	const FVoxelPinType BaseType;
	const FVoxelPinMetadata Metadata;

	bool IsPromotable() const
	{
		return BaseType.IsWildcard();
	}

	void SetType(const FVoxelPinType& NewType)
	{
		ensure(IsPromotable());
		ChildType = NewType;
	}
	const FVoxelPinType& GetType() const
	{
		ensure(BaseType == ChildType || IsPromotable());
		return ChildType;
	}

private:
	FVoxelPinType ChildType;

	FVoxelPin(
		const FName Name,
		const bool bIsInput,
		const float SortOrder,
		const FName VariadicPinName,
		const EVoxelPinFlags Flags,
		const FVoxelPinType& BaseType,
		const FVoxelPinType& ChildType,
		const FVoxelPinMetadata& Metadata)
		: Name(Name)
		, bIsInput(bIsInput)
		, SortOrder(SortOrder)
		, VariadicPinName(VariadicPinName)
		, Flags(Flags)
		, BaseType(BaseType)
		, Metadata(Metadata)
		, ChildType(ChildType)
	{
		ensure(BaseType.IsValid());
		ensure(ChildType.IsValid());
	}

	friend struct FVoxelNode;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

struct FVoxelPinRef
{
public:
	using Type = void;

	FVoxelPinRef() = default;
	explicit FVoxelPinRef(const FName Name)
		: Name(Name)
	{
	}
	operator FName() const
	{
		return Name;
	}

private:
	FName Name;
};

struct FVoxelVariadicPinRef
{
public:
	using Type = void;

	FVoxelVariadicPinRef() = default;
	explicit FVoxelVariadicPinRef(const FName Name)
		: Name(Name)
	{
	}
	operator FName() const
	{
		return Name;
	}

private:
	FName Name;
};

template<typename T>
struct TVoxelPinRef : FVoxelPinRef
{
	using Type = T;

	TVoxelPinRef() = default;
	explicit TVoxelPinRef(const FName Name)
		: FVoxelPinRef(Name)
	{
	}
};

template<>
struct TVoxelPinRef<FVoxelWildcard> : FVoxelPinRef
{
	using Type = void;

	TVoxelPinRef() = default;
	explicit TVoxelPinRef(const FName Name)
		: FVoxelPinRef(Name)
	{
	}
};

template<>
struct TVoxelPinRef<FVoxelWildcardBuffer> : FVoxelPinRef
{
	using Type = void;

	TVoxelPinRef() = default;
	explicit TVoxelPinRef(const FName Name)
		: FVoxelPinRef(Name)
	{
	}
};

template<typename T>
struct TVoxelVariadicPinRef : FVoxelVariadicPinRef
{
	using Type = T;

	TVoxelVariadicPinRef() = default;
	explicit TVoxelVariadicPinRef(const FName Name)
		: FVoxelVariadicPinRef(Name)
	{
	}
};

template<>
struct TVoxelVariadicPinRef<FVoxelWildcard> : FVoxelVariadicPinRef
{
	using Type = void;

	TVoxelVariadicPinRef() = default;
	explicit TVoxelVariadicPinRef(const FName Name)
		: FVoxelVariadicPinRef(Name)
	{
	}
};

template<>
struct TVoxelVariadicPinRef<FVoxelWildcardBuffer> : FVoxelVariadicPinRef
{
	using Type = void;

	TVoxelVariadicPinRef() = default;
	explicit TVoxelVariadicPinRef(const FName Name)
		: FVoxelVariadicPinRef(Name)
	{
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_UNIQUE_VOXEL_ID(FVoxelPinRuntimeId);

class VOXELGRAPHCORE_API FVoxelNodeRuntime final
	: public TSharedFromThis<FVoxelNodeRuntime>
	, public IVoxelNodeInterface
{
public:
	FVoxelNodeRuntime() = default;
	UE_NONCOPYABLE(FVoxelNodeRuntime);

	VOXEL_COUNT_INSTANCES();

	FVoxelFutureValue Get(
		const FVoxelPinRef& Pin,
		const FVoxelQuery& Query) const;

	TVoxelArray<FVoxelFutureValue> Get(
		const FVoxelVariadicPinRef& VariadicPin,
		const FVoxelQuery& Query) const;

public:
	template<typename T>
	TValue<T> Get(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const
	{
		return TValue<T>(Get(Pin, Query));
	}
	template<typename T, typename = std::enable_if_t<!std::is_same_v<T, FVoxelWildcard> && !std::is_same_v<T, FVoxelWildcardBuffer>>>
	TValue<T> Get(const TVoxelPinRef<T>& Pin, const FVoxelQuery& Query) const
	{
		return this->Get<T>(static_cast<const FVoxelPinRef&>(Pin), Query);
	}

	template<typename T>
	TVoxelArray<TValue<T>> Get(
		const TVoxelVariadicPinRef<T>& VariadicPin,
		const FVoxelQuery& Query) const
	{
		TVoxelArray<FVoxelFutureValue> Result = this->Get(FVoxelVariadicPinRef(VariadicPin), Query);
		for (const FVoxelFutureValue& Value : Result)
		{
			checkVoxelSlow(Value.GetParentType().CanBeCastedTo<T>());
		}
		return ReinterpretCastVoxelArray<TValue<T>>(MoveTemp(Result));
	}

public:
	virtual const FVoxelGraphNodeRef& GetNodeRef() const override
	{
		return NodeRef;
	}
	bool IsCallNode() const
	{
		return bIsCallNode;
	}

	TSharedRef<const FVoxelComputeValue> GetCompute(
		const FVoxelPinRef& Pin,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance) const;

	FVoxelDynamicValueFactory MakeDynamicValueFactory(const FVoxelPinRef& Pin) const;

	template<typename T, typename = std::enable_if_t<!std::is_same_v<T, FVoxelWildcard> && !std::is_same_v<T, FVoxelWildcardBuffer>>>
	TSharedRef<const TVoxelComputeValue<T>> GetCompute(
		const TVoxelPinRef<T>& Pin,
		const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance) const
	{
		return ReinterpretCastSharedRef<const TVoxelComputeValue<T>>(this->GetCompute(FVoxelPinRef(Pin), TerminalGraphInstance));
	}
	template<typename T>
	TVoxelDynamicValueFactory<T> MakeDynamicValueFactory(const TVoxelPinRef<T>& Pin) const
	{
		return TVoxelDynamicValueFactory<T>(this->MakeDynamicValueFactory(FVoxelPinRef(Pin)));
	}

public:
	struct FPinData : TSharedFromThis<FPinData>
	{
		const FVoxelPinType Type;
		const bool bIsInput;
		const FName StatName;
		const FVoxelPinRuntimeId PinId;
		const FVoxelPinMetadataFlags Metadata;

		TSharedPtr<const FVoxelComputeValue> Compute;

		FPinData(
			const FVoxelPinType& Type,
			bool bIsInput,
			FName StatName,
			const FVoxelGraphPinRef& PinRef,
			const FVoxelPinMetadataFlags& Metadata);
	};
	const FPinData& GetPinData(const FName PinName) const
	{
		return *NameToPinData[PinName];
	}
	const TVoxelMap<FName, TSharedPtr<FPinData>>& GetNameToPinData() const
	{
		return NameToPinData;
	}
	const TVoxelMap<FName, TVoxelArray<FName>>& GetVariadicPinNameToPinNames() const
	{
		return VariadicPinNameToPinNames;
	}

private:
	const FVoxelNode* Node = nullptr;
	FVoxelGraphNodeRef NodeRef;
	TOptional<bool> AreTemplatePinsBuffers;

	bool bIsCallNode = false;

	TVoxelMap<FName, TVoxelArray<FName>> VariadicPinNameToPinNames;
	TVoxelMap<FName, TSharedPtr<FPinData>> NameToPinData;

	friend FVoxelNode;
	friend class FVoxelNodeCaller;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename NodeType>
struct TVoxelNodeRuntimeForward : public NodeType
{
	FORCEINLINE const FVoxelNodeRuntime& GetNodeRuntime() const
	{
		return static_cast<const NodeType*>(this)->GetNodeRuntime();
	}

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelPinRef>::Value>>
	FORCEINLINE auto Get(const T& Pin, const FVoxelQuery& Query) const -> decltype(auto)
	{
		return GetNodeRuntime().Get(Pin, Query);
	}
	template<typename T, typename = std::enable_if_t<!TIsDerivedFrom<T, FVoxelPinRef>::Value>>
	FORCEINLINE auto Get(const FVoxelPinRef& Pin, const FVoxelQuery& Query) const -> decltype(auto)
	{
		return GetNodeRuntime().template Get<T>(Pin, Query);
	}

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelVariadicPinRef>::Value>, typename = void>
	FORCEINLINE auto Get(const T& Pin, const FVoxelQuery& Query) const -> decltype(auto)
	{
		return GetNodeRuntime().Get(Pin, Query);
	}
	template<typename T, typename = std::enable_if_t<!TIsDerivedFrom<T, FVoxelVariadicPinRef>::Value>>
	FORCEINLINE auto Get(const FVoxelVariadicPinRef& Pin, const FVoxelQuery& Query) const -> decltype(auto)
	{
		return GetNodeRuntime().template Get<T>(Pin, Query);
	}

	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelPinRef>::Value>>
	FORCEINLINE auto GetCompute(const T& Pin, const TSharedRef<FVoxelTerminalGraphInstance>& TerminalGraphInstance) const -> decltype(auto)
	{
		return GetNodeRuntime().GetCompute(Pin, TerminalGraphInstance);
	}
	template<typename T, typename = std::enable_if_t<TIsDerivedFrom<T, FVoxelPinRef>::Value>>
	FORCEINLINE auto MakeDynamicValueFactory(const T& Pin) const -> decltype(auto)
	{
		return GetNodeRuntime().MakeDynamicValueFactory(Pin);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelNodeExposedPinValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Voxel")
	FName Name;

	UPROPERTY(EditAnywhere, Category = "Voxel")
	FVoxelPinValue Value;

	bool operator==(const FName OtherName) const
	{
		return Name == OtherName;
	}

	// Required to compare nodes
	friend uint32 GetTypeHash(const FVoxelNodeExposedPinValue& InValue)
	{
		return FVoxelUtilities::MurmurHash(InValue.Name);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

USTRUCT()
struct FVoxelNodeVariadicPinSerializedData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FName> PinNames;
};

USTRUCT()
struct FVoxelNodeSerializedData
{
	GENERATED_BODY()

	UPROPERTY()
	bool bIsValid = false;

	UPROPERTY()
	TMap<FName, FVoxelPinType> NameToPinType;

	UPROPERTY()
	TMap<FName, FVoxelNodeVariadicPinSerializedData> VariadicPinNameToSerializedData;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSet<FName> ExposedPins;

	UPROPERTY()
	TArray<FVoxelNodeExposedPinValue> ExposedPinsValues;
#endif

	// Required to compare nodes
	friend uint32 GetTypeHash(const FVoxelNodeSerializedData& Data)
	{
		return FVoxelUtilities::MurmurHashMulti(
			Data.NameToPinType.Num(),
			Data.VariadicPinNameToSerializedData.Num()
#if WITH_EDITOR
			, Data.ExposedPins.Num()
			, Data.ExposedPinsValues.Num()
#endif
		);
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

using FVoxelNodeComputePtr = FVoxelFutureValue (*) (const FVoxelNode& Node, const FVoxelQuery& Query);

VOXELGRAPHCORE_API void RegisterVoxelNodeComputePtr(
	const UScriptStruct* Node,
	FName PinName,
	FVoxelNodeComputePtr Ptr,
	int32 Line);

#if WITH_EDITOR
VOXELGRAPHCORE_API bool FindVoxelNodeComputeLine(
	const UScriptStruct* Node,
	int32& OutLine);
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DECLARE_VOXEL_MEMORY_STAT(VOXELGRAPHCORE_API, STAT_VoxelNodes, "Nodes");

USTRUCT(meta = (Abstract))
struct VOXELGRAPHCORE_API FVoxelNode
	: public FVoxelVirtualStruct
	, public IVoxelNodeInterface
{
	GENERATED_BODY()
	DECLARE_VIRTUAL_STRUCT_PARENT(FVoxelNode, GENERATED_VOXEL_NODE_BODY)

public:
	FVoxelNode() = default;
	FVoxelNode(const FVoxelNode& Other) = delete;
	FVoxelNode& operator=(const FVoxelNode& Other);

	VOXEL_COUNT_INSTANCES();
	VOXEL_ALLOCATED_SIZE_TRACKER(STAT_VoxelNodes);

	virtual int64 GetAllocatedSize() const;

	//~ Begin IVoxelNodeInterface Interface
	FORCEINLINE virtual const FVoxelGraphNodeRef& GetNodeRef() const final override
	{
		return GetNodeRuntime().GetNodeRef();
	}
	//~ End IVoxelNodeInterface Interface

public:
#if WITH_EDITOR
	virtual UStruct& GetMetadataContainer() const;

	virtual FString GetCategory() const;
	virtual FString GetDisplayName() const;
	virtual FString GetTooltip() const;
#endif

	virtual bool ShowPromotablePinsAsWildcards() const
	{
		return true;
	}
	virtual bool IsPureNode() const
	{
		return false;
	}

	virtual void ReturnToPool();

	virtual void PreCompile() {}
	virtual FVoxelComputeValue CompileCompute(FName PinName) const;

	virtual uint32 GetNodeHash() const;
	virtual bool IsNodeIdentical(const FVoxelNode& Other) const;

public:
	virtual void PreSerialize() override;
	virtual void PostSerialize() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {}
#endif

private:
	using FVersion = DECLARE_VOXEL_VERSION
	(
		FirstVersion,
		AddIsValid
	);

	UPROPERTY()
	int32 Version = FVersion::FirstVersion;

public:
#if WITH_EDITOR
	using FDefinition = FVoxelNodeDefinition;
	virtual TSharedRef<FVoxelNodeDefinition> GetNodeDefinition();
#endif

#if WITH_EDITOR
	// Pin will always be a promotable pin
	virtual FVoxelPinTypeSet GetPromotionTypes(const FVoxelPin& Pin) const;
	virtual FString GetPinPromotionWarning(const FVoxelPin& Pin, const FVoxelPinType& NewType) const { return {}; }
	virtual void PromotePin(FVoxelPin& Pin, const FVoxelPinType& NewType);
#endif
	virtual void PromotePin_Runtime(FVoxelPin& Pin, const FVoxelPinType& NewType);

	FORCEINLINE bool AreTemplatePinsBuffers() const
	{
		const TOptional<bool> Value =
			NodeRuntime
			? NodeRuntime->AreTemplatePinsBuffers
			: AreTemplatePinsBuffersImpl();

		ensure(Value.IsSet());
		return Value.Get(false);
	}
	TOptional<bool> AreTemplatePinsBuffersImpl() const;

#if WITH_EDITOR
	bool IsPinHidden(const FVoxelPin& Pin) const;
	FString GetPinDefaultValue(const FVoxelPin& Pin) const;
	void UpdatePropertyBoundDefaultValue(const FVoxelPin& Pin, const FVoxelPinValue& NewValue);
#endif

public:
	template<typename T>
	struct TPinIterator
	{
		TVoxelMap<FName, TSharedPtr<FVoxelPin>>::FConstIterator Iterator;

		TPinIterator(TVoxelMap<FName, TSharedPtr<FVoxelPin>>::FConstIterator&& Iterator)
			: Iterator(Iterator)
		{
		}

		TPinIterator& operator++()
		{
			++Iterator;
			return *this;
		}
		explicit operator bool() const
		{
			return bool(Iterator);
		}
		T& operator*() const
		{
			return *Iterator.Value();
		}

		friend bool operator!=(const TPinIterator& Lhs, const TPinIterator& Rhs)
		{
			return Lhs.Iterator != Rhs.Iterator;
		}
	};
	template<typename T>
	struct TPinView
	{
		const TVoxelMap<FName, TSharedPtr<FVoxelPin>>& Pins;

		TPinView() = default;
		TPinView(const TVoxelMap<FName, TSharedPtr<FVoxelPin>>& Pins)
			: Pins(Pins)
		{
		}

		TPinIterator<T> begin() const { return TPinIterator<T>{ Pins.begin() }; }
		TPinIterator<T> end() const { return TPinIterator<T>{ Pins.end() }; }
	};

	TPinView<FVoxelPin> GetPins()
	{
		FlushDeferredPins();
		return TPinView<FVoxelPin>(PrivateNameToPin);
	}
	TPinView<const FVoxelPin> GetPins() const
	{
		FlushDeferredPins();
		return TPinView<const FVoxelPin>(PrivateNameToPin);
	}

	const TVoxelMap<FName, TSharedPtr<FVoxelPin>>& GetNameToPin()
	{
		FlushDeferredPins();
		return PrivateNameToPin;
	}
	const TVoxelMap<FName, TSharedPtr<const FVoxelPin>>& GetNameToPin() const
	{
		FlushDeferredPins();
		return ReinterpretCastRef<TVoxelMap<FName, TSharedPtr<const FVoxelPin>>>(PrivateNameToPin);
	}

	TSharedPtr<FVoxelPin> FindPin(const FName Name)
	{
		return GetNameToPin().FindRef(Name);
	}
	TSharedPtr<const FVoxelPin> FindPin(const FName Name) const
	{
		return GetNameToPin().FindRef(Name);
	}

	FVoxelPin& GetPin(const FVoxelPinRef& Pin)
	{
		return *GetNameToPin().FindChecked(Pin);
	}
	const FVoxelPin& GetPin(const FVoxelPinRef& Pin) const
	{
		return *GetNameToPin().FindChecked(Pin);
	}

	FVoxelPin& GetUniqueInputPin();
	FVoxelPin& GetUniqueOutputPin();

	const FVoxelPin& GetUniqueInputPin() const
	{
		return ConstCast(this)->GetUniqueInputPin();
	}
	const FVoxelPin& GetUniqueOutputPin() const
	{
		return ConstCast(this)->GetUniqueOutputPin();
	}

public:
	FName Variadic_AddPin(FName VariadicPinName, FName PinName = {});
	FName Variadic_InsertPin(FName VariadicPinName, int32 Position);
	FName Variadic_AddPin(FVoxelPinRef, FName = {}) = delete;

	const TVoxelArray<FVoxelPinRef>& GetVariadicPinPinNames(const FVoxelVariadicPinRef& VariadicPin) const
	{
		FlushDeferredPins();
		return ReinterpretCastVoxelArray<FVoxelPinRef>(PrivateNameToVariadicPin[VariadicPin]->Pins);
	}
	template<typename T>
	const TArray<TVoxelPinRef<T>>& GetVariadicPinPinNames(const TVoxelVariadicPinRef<T>& VariadicPin) const
	{
		return ReinterpretCastArray<TVoxelPinRef<T>>(GetVariadicPinPinNames(static_cast<const FVoxelVariadicPinRef&>(VariadicPin)));
	}

private:
	void FixupVariadicPinNames(FName VariadicPinName);

protected:
	FName CreatePin(
		const FVoxelPinType& Type,
		bool bIsInput,
		FName Name,
		const FVoxelPinMetadata& Metadata = {},
		EVoxelPinFlags Flags = EVoxelPinFlags::None,
		int32 MinVariadicNum = 0);

	void RemovePin(FName Name);

protected:
	FVoxelPinRef CreateInputPin(
		const FVoxelPinType& Type,
		const FName Name,
		const FVoxelPinMetadata& Metadata = {},
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return FVoxelPinRef(CreatePin(
			Type,
			true,
			Name,
			Metadata,
			Flags));
	}
	FVoxelPinRef CreateOutputPin(
		const FVoxelPinType& Type,
		const FName Name,
		const FVoxelPinMetadata& Metadata = {},
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return FVoxelPinRef(CreatePin(
			Type,
			false,
			Name,
			Metadata,
			Flags));
	}

protected:
	template<typename Type>
	TVoxelPinRef<Type> CreateInputPin(
		const FName Name,
		const FVoxelPinMetadata& Metadata,
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return TVoxelPinRef<Type>(this->CreateInputPin(
			FVoxelPinType::Make<Type>(),
			Name,
			Metadata,
			Flags));
	}
	template<typename Type>
	TVoxelPinRef<Type> CreateOutputPin(
		const FName Name,
		const FVoxelPinMetadata& Metadata,
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return TVoxelPinRef<Type>(this->CreateOutputPin(
			FVoxelPinType::Make<Type>(),
			Name,
			Metadata,
			Flags));
	}

protected:
	FVoxelVariadicPinRef CreateVariadicInputPin(
		const FVoxelPinType& Type,
		const FName Name,
		const FVoxelPinMetadata& Metadata,
		const int32 MinNum,
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return FVoxelVariadicPinRef(CreatePin(
			Type,
			true,
			Name,
			Metadata,
			Flags | EVoxelPinFlags::VariadicPin,
			MinNum));
	}
	template<typename Type>
	TVoxelVariadicPinRef<Type> CreateVariadicInputPin(
		const FName Name,
		const FVoxelPinMetadata& Metadata,
		const int32 MinNum,
		const EVoxelPinFlags Flags = EVoxelPinFlags::None)
	{
		return TVoxelVariadicPinRef<Type>(this->CreateVariadicInputPin(FVoxelPinType::Make<Type>(), Name, Metadata, MinNum, Flags));
	}

protected:
	struct FDeferredPin
	{
		FName VariadicPinName;
		int32 MinVariadicNum = 0;

		FName Name;
		bool bIsInput = false;
		float SortOrder = 0.f;
		EVoxelPinFlags Flags = {};
		FVoxelPinType BaseType;
		FVoxelPinType ChildType;
		FVoxelPinMetadata Metadata;

		bool IsVariadicRoot() const
		{
			return EnumHasAllFlags(Flags, EVoxelPinFlags::VariadicPin);
		}
		bool IsVariadicChild() const
		{
			return !VariadicPinName.IsNone();
		}
	};
	struct FVariadicPin
	{
		const FDeferredPin PinTemplate;
		TVoxelArray<FName> Pins;

		explicit FVariadicPin(const FDeferredPin& PinTemplate)
			: PinTemplate(PinTemplate)
		{
		}
	};

private:
	bool bEditorDataRemoved = false;

	int32 SortOrderCounter = 1;

	bool bIsDeferringPins = true;
	TVoxelArray<FDeferredPin> DeferredPins;

	TVoxelMap<FName, FDeferredPin> PrivateNameToPinBackup;
	TVoxelArray<FName> PrivatePinsOrder;
	int32 DisplayLastPins = 0;
	TVoxelMap<FName, TSharedPtr<FVoxelPin>> PrivateNameToPin;
	TVoxelMap<FName, TSharedPtr<FVariadicPin>> PrivateNameToVariadicPin;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Voxel", Transient)
	TArray<FVoxelNodeExposedPinValue> ExposedPinValues;

	TSet<FName> ExposedPins;
	FSimpleMulticastDelegate OnExposedPinsUpdated;
#endif

	FORCEINLINE void FlushDeferredPins() const
	{
		ensure(!bEditorDataRemoved);

		if (bIsDeferringPins)
		{
			ConstCast(this)->FlushDeferredPinsImpl();
		}
	}
	void FlushDeferredPinsImpl();
	void RegisterPin(FDeferredPin Pin, bool bApplyMinNum = true);
	void SortPins();
	void SortVariadicPinNames(FName VariadicPinName);

private:
	UPROPERTY()
	FVoxelNodeSerializedData SerializedDataProperty;

	FVoxelNodeSerializedData GetSerializedData() const;
	void LoadSerializedData(const FVoxelNodeSerializedData& SerializedData);

public:
	void InitializeNodeRuntime(
		const FVoxelGraphNodeRef& NodeRef,
		bool bIsCallNode);
	void RemoveEditorData();
	void EnableSharedNode(const TSharedRef<FVoxelNode>& SharedThis);

	FORCEINLINE bool HasNodeRuntime() const
	{
		return NodeRuntime.IsValid();
	}
	FORCEINLINE const FVoxelNodeRuntime& GetNodeRuntime() const
	{
		return *NodeRuntime;
	}
	template<typename T = FVoxelNode>
	FORCEINLINE TSharedRef<T> SharedNode() const
	{
		return CastChecked<T>(WeakThis.Pin().ToSharedRef());
	}

private:
	TSharedPtr<FVoxelNodeRuntime> NodeRuntime;
	TWeakPtr<FVoxelNode> WeakThis;

protected:
	using FReturnToPoolFunc = void (FVoxelNode::*)();

	void AddReturnToPoolFunc(const FReturnToPoolFunc ReturnToPool)
	{
		ReturnToPoolFuncs.Add(ReturnToPool);
	}

private:
	TArray<FReturnToPoolFunc> ReturnToPoolFuncs;

	friend class SVoxelGraphNode;
	friend class FVoxelNodeDefinition;
	friend class FVoxelGraphNode_Struct_Customization;
	friend class FVoxelGraphNodeVariadicPinCustomization;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GENERATED_VOXEL_NODE_DEFINITION_BODY(NodeType) \
	NodeType& Node; \
	explicit FDefinition(NodeType& Node) \
		: Super::FDefinition(Node) \
		, Node(Node) \
	{}

#if WITH_EDITOR
class VOXELGRAPHCORE_API FVoxelNodeDefinition : public IVoxelNodeDefinition
{
public:
	FVoxelNode& Node;

	explicit FVoxelNodeDefinition(FVoxelNode& Node)
		: Node(Node)
	{
	}

	virtual void Initialize(UEdGraphNode& EdGraphNode) {}

	virtual TSharedPtr<const FNode> GetInputs() const override;
	virtual TSharedPtr<const FNode> GetOutputs() const override;
	TSharedPtr<const FNode> GetPins(bool bIsInput) const;

	virtual FString GetAddPinLabel() const override;
	virtual FString GetAddPinTooltip() const override;
	virtual FString GetRemovePinTooltip() const override;

	virtual bool Variadic_CanAddPinTo(FName VariadicPinName) const override;
	virtual FName Variadic_AddPinTo(FName VariadicPinName) override;

	virtual bool Variadic_CanRemovePinFrom(FName VariadicPinName) const override;
	virtual void Variadic_RemovePinFrom(FName VariadicPinName) override;

	virtual bool CanRemoveSelectedPin(FName PinName) const override;
	virtual void RemoveSelectedPin(FName PinName) override;

	virtual void InsertPinBefore(FName PinName) override;
	virtual void DuplicatePin(FName PinName) override;

	virtual void ExposePin(FName PinName) override;
};
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if WITH_EDITOR
#define IMPL_GENERATED_VOXEL_NODE_EDITOR_BODY() \
	virtual TSharedRef<FVoxelNodeDefinition> GetNodeDefinition() override { return MakeVoxelShared<FDefinition>(*this); }
#else
#define IMPL_GENERATED_VOXEL_NODE_EDITOR_BODY()
#endif

#define GENERATED_VOXEL_NODE_BODY() \
	GENERATED_VIRTUAL_STRUCT_BODY_IMPL(FVoxelNode) \
	IMPL_GENERATED_VOXEL_NODE_EDITOR_BODY()

namespace FVoxelPinMetadataBuilder
{
	struct ArrayPin {};
	// If the sub-graph changes, the value will update but this node won't be re-created
	struct VirtualPin {};
	// For exec nodes only. If set, the pin value will be computed before Create is called
	// Implies VirtualPin
	struct ConstantPin {};
	struct OptionalPin {};
	struct DisplayLast {};
	// Will error out if pin is unlinked
	struct NoDefault {};
	struct ShowInDetail {};
	struct AdvancedDisplay {};

	template<typename>
	struct BaseClass {};

	namespace Internal
	{
		struct None {};

		template<typename T>
		struct TStringParam
		{
#if WITH_EDITOR
			FString Value;
#endif

			template<typename CharType>
			explicit TStringParam(const CharType* Value)
#if WITH_EDITOR
				: Value(Value)
#endif
			{
			}
			explicit TStringParam(const FString& Value)
#if WITH_EDITOR
				: Value(Value)
#endif
			{
			}

			T operator()() const { return ReinterpretCastRef<const T&>(*this); }
		};
	}

	struct DisplayName : Internal::TStringParam<DisplayName> { using TStringParam::TStringParam; };
	struct Tooltip : Internal::TStringParam<Tooltip> { using TStringParam::TStringParam; };
	struct Category : Internal::TStringParam<Category> { using TStringParam::TStringParam; };

	template<typename Type>
	struct TBuilder
	{
		static void MakeImpl(FVoxelPinMetadata&, Internal::None) {}

		static void MakeImpl(FVoxelPinMetadata& Metadata, DisplayName Value)
		{
#if WITH_EDITOR
			ensure(Metadata.DisplayName.IsEmpty());
			Metadata.DisplayName = Value.Value;
#endif
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, Category Value)
		{
#if WITH_EDITOR
			ensure(Metadata.Category.IsEmpty());
			Metadata.Category = Value.Value;
#endif
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, Tooltip Value)
		{
#if WITH_EDITOR
			ensure(Metadata.Category.IsEmpty());
			Metadata.Tooltip = Value.Value;
#endif
		}

		static void MakeImpl(FVoxelPinMetadata& Metadata, ArrayPin)
		{
			ensure(!Metadata.bArrayPin);
			Metadata.bArrayPin = true;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, VirtualPin)
		{
			ensure(!Metadata.bVirtualPin);
			Metadata.bVirtualPin = true;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, ConstantPin)
		{
			ensure(!Metadata.bVirtualPin);
			Metadata.bVirtualPin = true;
			ensure(!Metadata.bConstantPin);
			Metadata.bConstantPin = true;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, OptionalPin)
		{
			ensure(!Metadata.bOptionalPin);
			Metadata.bOptionalPin = true;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, DisplayLast)
		{
			ensure(!Metadata.bDisplayLast);
			Metadata.bDisplayLast = true;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, NoDefault)
		{
			ensure(!Metadata.bNoDefault);
			Metadata.bNoDefault = true;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, ShowInDetail)
		{
			ensure(!Metadata.bShowInDetail);
			Metadata.bShowInDetail = true;
		}
		static void MakeImpl(FVoxelPinMetadata& Metadata, AdvancedDisplay)
		{
#if WITH_EDITOR
			ensure(Metadata.Category.IsEmpty());
			Metadata.Category = "Advanced";
#endif
		}

		template<typename Class>
		static void MakeImpl(FVoxelPinMetadata& Metadata, BaseClass<Class>)
		{
			ensure(!Metadata.BaseClass);
			Metadata.BaseClass = Class::StaticClass();
		}

		template<typename T>
		static constexpr bool IsValid = std::is_same_v<decltype(TBuilder::MakeImpl(DeclVal<FVoxelPinMetadata&>(), DeclVal<T>())), void>;

		template<typename T, typename... ArgTypes, typename = std::enable_if_t<(... && IsValid<ArgTypes>)>>
		static FVoxelPinMetadata Make(
			const int32 Line,
			UScriptStruct* Struct,
			const T& DefaultValue,
			ArgTypes... Args)
		{
			FVoxelPinMetadata Metadata;
#if WITH_EDITOR
			ensure(Metadata.DefaultValue.IsEmpty());
			Metadata.DefaultValue = FVoxelNodeDefaultValueHelper::Get(static_cast<Type*>(nullptr), DefaultValue);
			Metadata.Line = Line;
			Metadata.Struct = Struct;
#endif
			VOXEL_FOLD_EXPRESSION(TBuilder::MakeImpl(Metadata, Args));
			return Metadata;
		}
	};
}

#define INTERNAL_VOXEL_PIN_METADATA_FOREACH(X) FVoxelPinMetadataBuilder::X()

#define VOXEL_PIN_METADATA_IMPL(Type, Line, Struct, Default, ...) \
	FVoxelPinMetadataBuilder::TBuilder<Type>::Make(Line, Struct, Default, VOXEL_FOREACH_COMMA(INTERNAL_VOXEL_PIN_METADATA_FOREACH, __VA_ARGS__))

#define VOXEL_PIN_METADATA(Type, Default, ...) VOXEL_PIN_METADATA_IMPL(Type, 0, nullptr, Default, Internal::None, ##__VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define INTERNAL_DECLARE_VOXEL_PIN(Type, Name) INTELLISENSE_ONLY(void VOXEL_APPEND_LINE(__DummyFunction)(FVoxelPinRef Name) { struct FVoxelWildcard {}; struct FVoxelWildcardBuffer {}; sizeof(Type); (void)Name; })

#define VOXEL_INPUT_PIN(InType, Name, Default, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(InType, Name); \
	TVoxelPinRef<InType> Name ## Pin = CreateInputPin<InType>(STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(InType, __LINE__, StaticStruct(), Default, Internal::None, ##__VA_ARGS__));

#define VOXEL_VARIADIC_INPUT_PIN(InType, Name, Default, MinNum, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(InType, Name); \
	TVoxelVariadicPinRef<InType> Name ## Pins = CreateVariadicInputPin<InType>(STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(InType, __LINE__, StaticStruct(), Default, Internal::None, ##__VA_ARGS__), MinNum);

#define VOXEL_OUTPUT_PIN(InType, Name, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(InType, Name); \
	TVoxelPinRef<InType> Name ## Pin = CreateOutputPin<InType>(STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(InType, __LINE__, StaticStruct(), nullptr, Internal::None, ##__VA_ARGS__));

///////////////////////////////////////////////////////////////////////////////
/////////////////// Template pins can be scalar or buffer /////////////////////
///////////////////////////////////////////////////////////////////////////////

#define VOXEL_TEMPLATE_INPUT_PIN(InType, Name, Default, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(InType, Name); \
	FVoxelPinRef Name ## Pin = ( \
		[] { checkStatic(!TIsVoxelBuffer<InType>::Value); }, \
		CreateInputPin(FVoxelPinType::Make<InType>().GetBufferType(), STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(InType, __LINE__, StaticStruct(), Default, Internal::None, ##__VA_ARGS__), EVoxelPinFlags::TemplatePin));

#define VOXEL_TEMPLATE_VARIADIC_INPUT_PIN(InType, Name, Default, MinNum, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(InType, Name); \
	FVoxelVariadicPinRef Name ## Pins = ( \
		[] { checkStatic(!TIsVoxelBuffer<InType>::Value); }, \
		CreateVariadicInputPin(FVoxelPinType::Make<InType>().GetBufferType(), STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(InType, __LINE__, StaticStruct(), Default, Internal::None, ##__VA_ARGS__), MinNum, EVoxelPinFlags::TemplatePin));

#define VOXEL_TEMPLATE_OUTPUT_PIN(InType, Name, ...) \
	INTERNAL_DECLARE_VOXEL_PIN(InType, Name); \
	FVoxelPinRef Name ## Pin = ( \
		[] { checkStatic(!TIsVoxelBuffer<InType>::Value); }, \
		CreateOutputPin(FVoxelPinType::Make<InType>().GetBufferType(), STATIC_FNAME(#Name), VOXEL_PIN_METADATA_IMPL(InType, __LINE__, StaticStruct(), nullptr, Internal::None, ##__VA_ARGS__), EVoxelPinFlags::TemplatePin));

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define VOXEL_PIN_NAME(NodeType, PinName) \
	([]() -> const auto& \
	 { \
		static const auto StaticName = VOXEL_ALLOW_MALLOC_INLINE(NodeType().PinName); \
		return StaticName; \
	 }())

#define VOXEL_CALL_PARAM(Type, Name) \
	Type Name = {}; \
	void Internal_ReturnToPool_ ## Name() \
	{ \
		Name = Type(); \
	} \
	VOXEL_ON_CONSTRUCT() \
	{ \
		using ThisType = VOXEL_THIS_TYPE; \
		AddReturnToPoolFunc(static_cast<FReturnToPoolFunc>(&ThisType::Internal_ReturnToPool_ ## Name)); \
	};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelNodeCaller
{
public:
	class VOXELGRAPHCORE_API FBindings
	{
	public:
		FVoxelComputeValue& Bind(FName Name) const;

		template<typename T>
		FORCEINLINE TVoxelComputeValue<T>& Bind(const TVoxelPinRef<T> Name) const
		{
			return ReinterpretCastRef<TVoxelComputeValue<T>>(Bind(FName(Name)));
		}

	private:
		FVoxelNodeRuntime& NodeRuntime;

		explicit FBindings(FVoxelNodeRuntime& NodeRuntime)
			: NodeRuntime(NodeRuntime)
		{
		}

		friend FVoxelNodeCaller;
	};

	struct VOXELGRAPHCORE_API FNodePool
	{
		FVoxelCriticalSection CriticalSection;
		TVoxelArray<TSharedRef<FVoxelNode>> Nodes;

		FNodePool();
	};

	template<typename NodeType, typename OutputType>
	struct TLambdaCaller
	{
		const FVoxelQuery& Query;
		const FVoxelPinRef OutputPin;

		template<typename LambdaType>
		TVoxelFutureValue<OutputType> operator+(LambdaType Lambda)
		{
			static FNodePool Pool;
			static const FName StatName = TEXTVIEW("Call ") + NodeType::StaticStruct()->GetFName();

			return TVoxelFutureValue<OutputType>(FVoxelNodeCaller::CallNode(
				Pool,
				NodeType::StaticStruct(),
				StatName,
				Query,
				OutputPin,
				[&](FBindings& Bindings, FVoxelNode& Node)
				{
					Lambda(Bindings, CastChecked<NodeType>(Node));
				}));
		}
	};

private:
	static FVoxelFutureValue CallNode(
		FNodePool& Pool,
		const UScriptStruct* Struct,
		FName StatName,
		const FVoxelQuery& Query,
		FVoxelPinRef OutputPin,
		TFunctionRef<void(FBindings&, FVoxelNode& Node)> Bind);
};

template<typename Type>
struct TVoxelCallNodeBindHelper
{
    constexpr Type operator()() const
	{
        return {};
    }
};

#define VOXEL_CALL_NODE(NodeType, OutputPin, Query) \
	FVoxelNodeCaller::TLambdaCaller<INTELLISENSE_SWITCH(void, NodeType), VOXEL_GET_TYPE(NodeType().OutputPin)::Type>{ Query, VOXEL_PIN_NAME(NodeType, OutputPin) } + \
		[&](FVoxelNodeCaller::FBindings& Bindings, NodeType& CalleeNode)

#define VOXEL_CALL_NODE_BIND(Name, ...) \
	Bindings.Bind(CalleeNode.Name) = [this, \
		ReturnPinType = CalleeNode.GetNodeRuntime().GetPinData(CalleeNode.Name).Type, \
		ReturnPinStatName = CalleeNode.GetNodeRuntime().GetPinData(CalleeNode.Name).StatName, \
		ReturnInnerType = TVoxelCallNodeBindHelper<VOXEL_GET_TYPE(CalleeNode.Name)::Type>(), \
		##__VA_ARGS__](const FVoxelQuery& Query) \
	-> \
	TChooseClass< \
		TIsDerivedFrom<VOXEL_GET_TYPE(CalleeNode.Name), FVoxelPinRef>::Value, \
		::TVoxelFutureValue<VOXEL_GET_TYPE(CalleeNode.Name)::Type>, \
		TArray<::TVoxelFutureValue<VOXEL_GET_TYPE(CalleeNode.Name)::Type>> \
	>::Result