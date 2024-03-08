// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelParameter.h"
#include "VoxelParameterPath.h"
#include "VoxelParameterOverridesOwner.generated.h"

class UVoxelGraph;
class FVoxelParameterView;
class FVoxelGraphParametersView;

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelParameterValueOverride
{
	GENERATED_BODY()

	UPROPERTY()
	bool bEnable = false;

	UPROPERTY()
	FVoxelPinValue Value;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FName CachedName;

	UPROPERTY()
	FString CachedCategory;
#endif
};

USTRUCT()
struct VOXELGRAPHCORE_API FVoxelParameterOverrides
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FVoxelParameterPath, FVoxelParameterValueOverride> PathToValueOverride;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API IVoxelParameterOverridesOwner
{
public:
	// Might be called often, not entirely accurate
	mutable FSimpleMulticastDelegate OnParameterValueChanged;
	// Called whenever any graph used in this override has a parameter change
	// This include graphs used by inline graphs
	// Will be fired after OnParameterValueChanged
	// Will be fired when GetGraph changes
	mutable FSimpleMulticastDelegate OnParameterLayoutChanged;

	IVoxelParameterOverridesOwner() = default;
	virtual ~IVoxelParameterOverridesOwner() = default;

	virtual UObject* _getUObject() const { return nullptr; }
	virtual TSharedPtr<IVoxelParameterOverridesOwner> AsShared() const = 0;

	virtual bool ShouldForceEnableOverride(const FVoxelParameterPath& Path) const = 0;
	virtual UVoxelGraph* GetGraph() const = 0;
	virtual FVoxelParameterOverrides& GetParameterOverrides() = 0;

#if WITH_EDITOR
	virtual void PreEditChangeOverrides() {}
	virtual void PostEditChangeOverrides() {}
#endif

public:
	FORCEINLINE const FVoxelParameterOverrides& GetParameterOverrides() const
	{
		return ConstCast(this)->GetParameterOverrides();
	}

	FORCEINLINE TMap<FVoxelParameterPath, FVoxelParameterValueOverride>& GetPathToValueOverride()
	{
		return GetParameterOverrides().PathToValueOverride;
	}
	FORCEINLINE const TMap<FVoxelParameterPath, FVoxelParameterValueOverride>& GetPathToValueOverride() const
	{
		return GetParameterOverrides().PathToValueOverride;
	}

public:
	void NotifyGraphChanged();
	// Call in PostInitProperties to ensure delegates are bound properly
	void FixupParameterOverrides();

public:
	TSharedPtr<FVoxelGraphParametersView> GetParametersView() const;
	TSharedRef<FVoxelGraphParametersView> GetParametersView_ValidGraph() const;

public:
	bool HasParameter(FName Name) const;

	FVoxelPinValue GetParameter(
		FName Name,
		FString* OutError = nullptr) const;

	FVoxelPinValue GetParameterTyped(
		FName Name,
		const FVoxelPinType& Type,
		FString* OutError = nullptr) const;

	bool SetParameter(
		FName Name,
		const FVoxelPinValue& Value,
		FString* OutError = nullptr);

public:
	// Use GetParameterChecked if possible
	template<typename T, typename ReturnType = std::decay_t<decltype(DeclVal<FVoxelPinValue>().Get<T>())>>
	TOptional<ReturnType> GetParameter(const FName Name, FString* OutError = nullptr)
	{
		const FVoxelPinValue Value = this->GetParameterTyped(Name, FVoxelPinType::Make<T>(), OutError);
		if (!Value.IsValid())
		{
			return {};
		}
		return Value.Get<T>();
	}
	template<typename T, typename ReturnType = std::decay_t<decltype(DeclVal<FVoxelPinValue>().Get<T>())>>
	ReturnType GetParameterChecked(const FName Name)
	{
		FString Error;
		const TOptional<ReturnType> Value = this->GetParameter<T>(Name, &Error);
		if (!ensureMsgf(Value.IsSet(), TEXT("%s"), *Error))
		{
			return {};
		}
		return Value.GetValue();
	}

	// Use SetParameterChecked if possible
	template<typename T, typename = std::enable_if_t<TIsSafeVoxelPinValue<T>::Value>>
	bool SetParameter(const FName Name, const T& Value, FString* OutError = nullptr)
	{
		return this->SetParameter(Name, FVoxelPinValue::Make(Value), OutError);
	}
	template<typename T, typename = std::enable_if_t<TIsSafeVoxelPinValue<T>::Value>>
	void SetParameterChecked(const FName Name, const T& Value)
	{
		FString Error;
		if (!this->SetParameter(Name, FVoxelPinValue::Make(Value), &Error))
		{
			ensureMsgf(false, TEXT("%s"), *Error);
		}
	}

private:
	FSharedVoidPtr OnParameterValueChangedPtr;
#if WITH_EDITOR
	FSharedVoidPtr OnParameterChangedPtr;
#endif
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

UINTERFACE()
class VOXELGRAPHCORE_API UVoxelParameterOverridesObjectOwner : public UInterface
{
	GENERATED_BODY()
};

class VOXELGRAPHCORE_API IVoxelParameterOverridesObjectOwner
	: public IInterface
#if CPP
	, public IVoxelParameterOverridesOwner
#endif
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual void PreEditChangeOverrides() final override;
	virtual void PostEditChangeOverrides() final override;
#endif

private:
	virtual TSharedPtr<IVoxelParameterOverridesOwner> AsShared() const final override
	{
		return nullptr;
	}
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class VOXELGRAPHCORE_API FVoxelParameterOverridesOwnerPtr
{
public:
	FVoxelParameterOverridesOwnerPtr() = default;
	FVoxelParameterOverridesOwnerPtr(IVoxelParameterOverridesOwner* Owner);

	FORCEINLINE bool IsValid() const
	{
		return
			WeakPtr.IsValid() ||
			ObjectPtr.IsValid();
	}
	FORCEINLINE IVoxelParameterOverridesOwner* Get() const
	{
		if (!IsValid())
		{
			return nullptr;
		}
		return OwnerPtr;
	}
	FORCEINLINE UObject* GetObject() const
	{
		return ObjectPtr.Get();
	}

	FORCEINLINE IVoxelParameterOverridesOwner* operator->() const
	{
		check(IsValid());
		return OwnerPtr;
	}
	FORCEINLINE IVoxelParameterOverridesOwner* operator*() const
	{
		check(IsValid());
		return OwnerPtr;
	}

private:
	IVoxelParameterOverridesOwner* OwnerPtr = nullptr;
	FWeakVoidPtr WeakPtr;
	FWeakObjectPtr ObjectPtr;
};