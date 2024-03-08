﻿// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "UObject/UObjectHash.h"
#include "UObject/WeakInterfacePtr.h"
#include "Templates/ChooseClass.h"
#include "VoxelMinimal/VoxelSharedPtr.h"
#include "VoxelMinimal/VoxelDereferencingIterator.h"
#include "VoxelMinimal/Containers/VoxelChunkedArray.h"

template<typename>
struct TIsSoftObjectPtr
{
	static constexpr bool Value = false;
};

template<>
struct TIsSoftObjectPtr<FSoftObjectPtr>
{
	static constexpr bool Value = true;
};

template<typename T>
struct TIsSoftObjectPtr<TSoftObjectPtr<T>>
{
	static constexpr bool Value = true;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename>
struct TSubclassOfType;

template<typename T>
struct TSubclassOfType<TSubclassOf<T>>
{
	using Type = T;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename FieldType>
FORCEINLINE FieldType* CastField(FField& Src)
{
	return CastField<FieldType>(&Src);
}
template<typename FieldType>
FORCEINLINE const FieldType* CastField(const FField& Src)
{
	return CastField<FieldType>(&Src);
}

template<typename FieldType>
FORCEINLINE FieldType& CastFieldChecked(FField& Src)
{
	return *CastFieldChecked<FieldType>(&Src);
}
template<typename FieldType>
FORCEINLINE const FieldType& CastFieldChecked(const FField& Src)
{
	return *CastFieldChecked<FieldType>(&Src);
}

template<typename To, typename From>
FORCEINLINE To* CastEnsured(From* Src)
{
	To* Result = Cast<To>(Src);
	ensure(!Src || Result);
	return Result;
}
template<typename To, typename From>
FORCEINLINE To* CastEnsured(const TObjectPtr<From>& Src)
{
	return CastEnsured<To>(Src.Get());
}

template<typename ToType, typename FromType, typename Allocator, typename = std::enable_if_t<TIsDerivedFrom<
	std::remove_const_t<ToType>,
	std::remove_const_t<FromType>
>::Value>>
FORCEINLINE const TArray<ToType*, Allocator>& CastChecked(const TArray<FromType*, Allocator>& Array)
{
#if VOXEL_DEBUG
	for (FromType* Element : Array)
	{
		checkVoxelSlow(Element->template IsA<ToType>());
	}
#endif

	return ReinterpretCastArray<ToType*>(Array);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename Class>
FORCEINLINE FName GetClassFName()
{
	// StaticClass is a few instructions for FProperties
	static FName StaticName;
	if (StaticName.IsNone())
	{
		StaticName = Class::StaticClass()->GetFName();
	}
	return StaticName;
}
template<typename Class>
FORCEINLINE FString GetClassName()
{
	return Class::StaticClass()->GetName();
}

template<typename Enum>
FORCEINLINE UEnum* StaticEnumFast()
{
	VOXEL_STATIC_HELPER(UEnum*)
	{
		StaticValue = StaticEnum<std::decay_t<Enum>>();
	}
	return StaticValue;
}
template<typename Struct>
FORCEINLINE UScriptStruct* StaticStructFast()
{
	VOXEL_STATIC_HELPER(UScriptStruct*)
	{
		StaticValue = TBaseStructure<std::decay_t<Struct>>::Get();
	}
	return StaticValue;
}
template<typename Class>
FORCEINLINE UClass* StaticClassFast()
{
	VOXEL_STATIC_HELPER(UClass*)
	{
		StaticValue = Class::StaticClass();
	}
	return StaticValue;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VOXELCORE_API void ForEachAssetOfClass(
	const UClass* ClassToLookFor,
	TFunctionRef<void(UObject*)> Operation);

template<typename T, typename LambdaType, typename = std::enable_if_t<std::is_void_v<decltype(DeclVal<LambdaType>()(DeclVal<T&>()))>>>
void ForEachAssetOfClass(LambdaType&& Operation)
{
	ForEachAssetOfClass(T::StaticClass(), [&](UObject* Asset)
	{
		Operation(*CastChecked<T>(Asset));
	});
}

template<typename T, typename LambdaType, typename = std::enable_if_t<std::is_void_v<decltype(DeclVal<LambdaType>()(DeclVal<T&>()))>>>
void ForEachObjectOfClass(LambdaType&& Operation, bool bIncludeDerivedClasses = true, EObjectFlags ExcludeFlags = RF_ClassDefaultObject, EInternalObjectFlags ExclusionInternalFlags = EInternalObjectFlags::None)
{
	ForEachObjectOfClass(T::StaticClass(), [&](UObject* Object)
	{
		checkVoxelSlow(Object && Object->IsA<T>());
		Operation(*static_cast<T*>(Object));
	}, bIncludeDerivedClasses, ExcludeFlags, ExclusionInternalFlags);
}

template<typename T, typename LambdaType, typename = std::enable_if_t<std::is_void_v<decltype(DeclVal<LambdaType>()(DeclVal<T&>()))>>>
void ForEachObjectOfClass_Copy(LambdaType&& Operation, bool bIncludeDerivedClasses = true, EObjectFlags ExcludeFlags = RF_ClassDefaultObject, EInternalObjectFlags ExclusionInternalFlags = EInternalObjectFlags::None)
{
	TVoxelChunkedArray<T*> Objects;
	ForEachObjectOfClass<T>([&](T& Object)
	{
		Objects.Add(&Object);
	}, bIncludeDerivedClasses, ExcludeFlags, ExclusionInternalFlags);

	for (T* Object : Objects)
	{
		Operation(*Object);
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T = void, typename ArrayType = typename TChooseClass<std::is_same_v<T, void>, UClass*, TSubclassOf<T>>::Result>
TArray<ArrayType> GetDerivedClasses(const UClass* BaseClass = T::StaticClass(), const bool bRecursive = true, const bool bRemoveDeprecated = true)
{
	VOXEL_FUNCTION_COUNTER();

	TArray<UClass*> Result;
	GetDerivedClasses(BaseClass, Result, bRecursive);

	if (bRemoveDeprecated)
	{
		Result.RemoveAllSwap([](const UClass* Class)
		{
			return Class->HasAnyClassFlags(CLASS_Deprecated);
		});
	}

	return ReinterpretCastArray<ArrayType>(MoveTemp(Result));
}

VOXELCORE_API TArray<UScriptStruct*> GetDerivedStructs(const UScriptStruct* BaseStruct, bool bIncludeBase = false);

template<typename T>
TArray<UScriptStruct*> GetDerivedStructs(const bool bIncludeBase = false)
{
	return GetDerivedStructs(T::StaticStruct(), bIncludeBase);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VOXELCORE_API bool IsFunctionInput(const FProperty& Property);
VOXELCORE_API TArray<UFunction*> GetClassFunctions(const UClass* Class, bool bIncludeSuper = false);

#if WITH_EDITOR
VOXELCORE_API FString GetStringMetaDataHierarchical(const UStruct* Struct, FName Name);
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VOXELCORE_API TVoxelDereferencingRange<TFieldRange<FProperty>> GetStructProperties(const UStruct& Struct);
VOXELCORE_API TVoxelDereferencingRange<TFieldRange<FProperty>> GetStructProperties(const UStruct* Struct);

VOXELCORE_API TVoxelDereferencingRange<TFieldRange<FProperty>> GetClassProperties(const UClass& Class);
VOXELCORE_API TVoxelDereferencingRange<TFieldRange<FProperty>> GetClassProperties(const UClass* Class);

VOXELCORE_API TVoxelDereferencingRange<TFieldRange<FProperty>> GetFunctionProperties(const UFunction& Function);
VOXELCORE_API TVoxelDereferencingRange<TFieldRange<FProperty>> GetFunctionProperties(const UFunction* Function);

template<typename T>
FORCEINLINE TVoxelDereferencingRange<TFieldRange<FProperty>> GetClassProperties()
{
	return GetClassProperties(StaticClassFast<T>());
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
FProperty& FindFPropertyChecked_Impl(const FName Name)
{
	UStruct* Struct;
	if constexpr (TIsDerivedFrom<T, UObject>::Value)
	{
		Struct = T::StaticClass();
	}
	else
	{
		Struct = StaticStructFast<T>();
	}

	FProperty* Property = FindFProperty<FProperty>(Struct, Name);
	check(Property);
	return *Property;
}

#define FindFPropertyChecked(Class, Name) FindFPropertyChecked_Impl<Class>(GET_MEMBER_NAME_CHECKED(Class, Name))

#define FindUFunctionChecked(Class, Name) \
	[] \
	{ \
		static UFunction* Function = Class::StaticClass()->FindFunctionByName(GET_FUNCTION_NAME_CHECKED(Class, Name)); \
		check(Function); \
		return Function; \
	}()

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

VOXELCORE_API FSharedVoidRef MakeSharedStruct(const UScriptStruct* Struct, const void* StructToCopyFrom = nullptr);
VOXELCORE_API FSharedVoidRef MakeShareableStruct(const UScriptStruct* Struct, void* StructMemory);

template<typename T>
TSharedRef<T> MakeSharedStruct(const UScriptStruct* Struct, const T* StructToCopyFrom = nullptr)
{
	check(Struct->IsChildOf(StaticStructFast<T>()));

	const TSharedRef<T> SharedRef = ReinterpretCastRef<TSharedRef<T>>(MakeSharedStruct(Struct, static_cast<const void*>(StructToCopyFrom)));
	SharedPointerInternals::EnableSharedFromThis(&SharedRef, &SharedRef.Get(), &SharedRef.Get());
	return SharedRef;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
FORCEINLINE T* ResolveObjectPtrFast(const TObjectPtr<T>& ObjectPtr)
{
	// Don't broadcast a delegate on every access

	if (IsObjectHandleResolved(ObjectPtr.GetHandle()))
	{
		return static_cast<T*>(UE::CoreUObject::Private::ReadObjectHandlePointerNoCheck(ObjectPtr.GetHandle()));
	}

	return SlowPath_ResolveObjectPtrFast(ObjectPtr);
}
template<typename T>
FORCENOINLINE T* SlowPath_ResolveObjectPtrFast(const TObjectPtr<T>& ObjectPtr)
{
	return ObjectPtr.Get();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

template<typename T>
FORCEINLINE FObjectKey MakeObjectKey(const TWeakObjectPtr<T>& Ptr)
{
	return ReinterpretCastRef<FObjectKey>(Ptr);
}

template<typename T>
FORCEINLINE TWeakObjectPtr<T> MakeWeakObjectPtr(T& Object)
{
	return TWeakObjectPtr<T>(&Object);
}

template<typename T>
FORCEINLINE TWeakInterfacePtr<T> MakeWeakInterfacePtr(T* Ptr)
{
	return TWeakInterfacePtr<T>(Ptr);
}
template<typename T, typename OtherType>
FORCEINLINE TWeakInterfacePtr<T> MakeWeakInterfacePtr(OtherType* Ptr)
{
	return TWeakInterfacePtr<T>(Ptr);
}
template<typename T, typename OtherType>
FORCEINLINE TWeakInterfacePtr<T> MakeWeakInterfacePtr(const TObjectPtr<OtherType> Ptr)
{
	return TWeakInterfacePtr<T>(Ptr.Get());
}
template<typename T>
FORCEINLINE TWeakInterfacePtr<T> MakeWeakInterfacePtr(const TScriptInterface<T>& Ptr)
{
	return TWeakInterfacePtr<T>(Ptr.GetInterface());
}

template<typename T>
FORCEINLINE uint32 GetTypeHash(const TWeakInterfacePtr<T>& Ptr)
{
	return GetTypeHash(Ptr.GetWeakObjectPtr());
}