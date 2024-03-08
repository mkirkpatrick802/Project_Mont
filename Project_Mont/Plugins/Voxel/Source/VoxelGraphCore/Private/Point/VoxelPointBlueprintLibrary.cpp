// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Point/VoxelPointBlueprintLibrary.h"
#include "Point/VoxelPointSet.h"
#include "Point/VoxelPointStorage.h"
#include "Point/VoxelPointStorageData.h"
#include "Point/VoxelPointHandleProvider.h"
#include "Point/VoxelPointManager.h"
#include "VoxelRuntime.h"

EVoxelSuccess UVoxelPointBlueprintLibrary::MakePointHandleFromHitResult(const FHitResult& HitResult, FVoxelPointHandle& Handle)
{
	VOXEL_FUNCTION_COUNTER();

	Handle = {};

	if (!HitResult.bBlockingHit)
	{
		return EVoxelSuccess::Failed;
	}

	const UPrimitiveComponent* Component = HitResult.GetComponent();
	if (!Component)
	{
		return EVoxelSuccess::Failed;
	}

	const IVoxelPointHandleProvider* Provider = Cast<IVoxelPointHandleProvider>(Component);
	if (!Provider)
	{
		return EVoxelSuccess::Failed;
	}

	if (!Provider->TryGetPointHandle(HitResult.Item, Handle))
	{
		return EVoxelSuccess::Failed;
	}

	return EVoxelSuccess::Succeeded;
}

EVoxelSuccess UVoxelPointBlueprintLibrary::MakePointHandleFromOverlapResult(const FOverlapResult& OverlapResult, FVoxelPointHandle& Handle)
{
	VOXEL_FUNCTION_COUNTER();

	Handle = {};

	if (!OverlapResult.bBlockingHit)
	{
		return EVoxelSuccess::Failed;
	}

	const UPrimitiveComponent* Component = OverlapResult.GetComponent();
	if (!Component)
	{
		return EVoxelSuccess::Failed;
	}

	const IVoxelPointHandleProvider* Provider = Cast<IVoxelPointHandleProvider>(Component);
	if (!Provider)
	{
		return EVoxelSuccess::Failed;
	}

	if (!Provider->TryGetPointHandle(OverlapResult.ItemIndex, Handle))
	{
		return EVoxelSuccess::Failed;
	}

	return EVoxelSuccess::Succeeded;
}

EVoxelSuccess UVoxelPointBlueprintLibrary::GetPointTransform(
	const FVoxelPointHandle& Handle,
	FTransform& Transform)
{
	VOXEL_FUNCTION_COUNTER();

	Transform = FTransform::Identity;

	if (!Handle.IsValid())
	{
		VOXEL_MESSAGE(Error, "Invalid handle");
		return EVoxelSuccess::Failed;
	}

	const UWorld* World = Handle.GetWorld();
	if (!World)
	{
		VOXEL_MESSAGE(Error, "Invalid world");
		return EVoxelSuccess::Failed;
	}

	FString Error;
	const TSharedPtr<FVoxelRuntime> Runtime = Handle.GetRuntime(&Error);
	if (!Runtime)
	{
		VOXEL_MESSAGE(Error, "Failed to resolve runtime: {0}", Error);
		return EVoxelSuccess::Failed;
	}

	FVoxelPinValue Position;
	FVoxelPinValue Rotation;
	FVoxelPinValue Scale;
	{
		VOXEL_SCOPE_COUNTER("Find cached");

		const TSharedRef<FVoxelPointManager> PointManager = FVoxelPointManager::Get(World);
		const FVoxelRuntimePinValue RuntimePosition = PointManager->FindAttribute(FVoxelPointAttributes::Position, Handle.ChunkRef, Handle.PointId);
		const FVoxelRuntimePinValue RuntimeRotation = PointManager->FindAttribute(FVoxelPointAttributes::Rotation, Handle.ChunkRef, Handle.PointId);
		const FVoxelRuntimePinValue RuntimeScale = PointManager->FindAttribute(FVoxelPointAttributes::Scale, Handle.ChunkRef, Handle.PointId);

		if (RuntimePosition.IsValid())
		{
			Position = FVoxelPinType::MakeExposedValue(RuntimePosition, false);
			ensure(Position.IsValid());
		}
		if (RuntimeRotation.IsValid())
		{
			Rotation = FVoxelPinType::MakeExposedValue(RuntimeRotation, false);
			ensure(Rotation.IsValid());
		}
		if (RuntimeScale.IsValid())
		{
			Scale = FVoxelPinType::MakeExposedValue(RuntimeScale, false);
			ensure(Scale.IsValid());
		}
	}

	TVoxelMap<FName, FVoxelPinValue> Attributes;
	if (!Position.IsValid())
	{
		Attributes.Add_CheckNew(FVoxelPointAttributes::Position);
	}
	if (!Rotation.IsValid())
	{
		Attributes.Add_CheckNew(FVoxelPointAttributes::Rotation);
	}
	if (!Scale.IsValid())
	{
		Attributes.Add_CheckNew(FVoxelPointAttributes::Scale);
	}

	if (Attributes.Num() > 0)
	{
		VOXEL_SCOPE_COUNTER("GetAttributes");

		if (!Handle.GetAttributes(Attributes, &Error))
		{
			VOXEL_MESSAGE(Error, "Failed to get attributes: {0}", Error);
			return EVoxelSuccess::Failed;
		}

		if (!Position.IsValid())
		{
			Position = Attributes.FindChecked(FVoxelPointAttributes::Position);
		}
		if (!Rotation.IsValid())
		{
			Rotation = Attributes.FindChecked(FVoxelPointAttributes::Rotation);
		}
		if (!Scale.IsValid())
		{
			Scale = Attributes.FindChecked(FVoxelPointAttributes::Scale);
		}
	}

	if (ensure(Position.Is<FVector>()))
	{
		Transform.SetLocation(Position.Get<FVector>());
	}
	if (ensure(Rotation.Is<FQuat>()))
	{
		Transform.SetRotation(Rotation.Get<FQuat>());
	}
	if (ensure(Scale.Is<FVector>()))
	{
		Transform.SetScale3D(Scale.Get<FVector>());
	}

	Transform *= FTransform(Runtime->GetLocalToWorld().Get_NoDependency());
	return EVoxelSuccess::Succeeded;
}

bool UVoxelPointBlueprintLibrary::HasPointAttribute(
	const FVoxelPointHandle& Handle,
	const FName Name)
{
	FString Error;
	const FVoxelPinValue Value = GetPointAttribute(
		Handle,
		Name,
		&Error);

	return Value.IsValid();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void UVoxelPointBlueprintLibrary::SetPointVisibility(
	const FVoxelPointHandle& Handle,
	const bool bVisible)
{
	BulkSetPointVisibility(
		Handle.ChunkRef,
		TVoxelArray<FVoxelPointId>{ Handle.PointId },
		bVisible);
}

void UVoxelPointBlueprintLibrary::BulkSetPointVisibility(
	const FVoxelPointChunkRef& ChunkRef,
	const TConstVoxelArrayView<FVoxelPointId> PointIds,
	const bool bVisible)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ChunkRef.IsValid())
	{
		VOXEL_MESSAGE(Error, "Invalid handle");
		return;
	}

	const UWorld* World = ChunkRef.GetWorld();
	if (!World)
	{
		VOXEL_MESSAGE(Error, "Invalid world");
		return;
	}

	const TSharedRef<FVoxelPointOverrideChunk> Chunk = FVoxelPointManager::Get(World)->FindOrAddChunk(ChunkRef);

	TMulticastDelegate<void(TConstVoxelArrayView<FVoxelPointId>)> OnChanged;
	{
		VOXEL_SCOPE_LOCK(Chunk->CriticalSection);

		if (bVisible)
		{
			for (const FVoxelPointId& PointId : PointIds)
			{
				Chunk->PointIdsToHide_RequiresLock.Remove(PointId);
			}
		}
		else
		{
			Chunk->PointIdsToHide_RequiresLock.Append(PointIds);
		}

		OnChanged = Chunk->OnChanged_RequiresLock;
	}

	VOXEL_SCOPE_COUNTER("OnChanged");
	OnChanged.Broadcast(PointIds);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool UVoxelPointBlueprintLibrary::SetPointAttribute(
	const FVoxelPointHandle& Handle,
	const FName Name,
	const FVoxelPinType& Type,
	const FVoxelPinValue& Value,
	FString* OutError)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_COUNTER_FNAME(Name);

	if (!Handle.IsValid())
	{
		if (OutError)
		{
			*OutError = "Invalid handle";
		}
		return false;
	}

	const TSharedPtr<FVoxelRuntime> Runtime = Handle.GetRuntime(OutError);
	if (!Runtime)
	{
		return false;
	}

	const TSharedPtr<const FVoxelRuntimeParameter_PointStorage> Parameter = Runtime->FindParameter<FVoxelRuntimeParameter_PointStorage>();
	if (!Parameter ||
		!ensure(Parameter->Data))
	{
		if (OutError)
		{
			*OutError = "Runtime has no point storage";
		}
		return false;
	}

	return Parameter->Data->SetPointAttribute(
		Handle,
		Name,
		Type,
		Value,
		OutError);
}

DEFINE_FUNCTION(UVoxelPointBlueprintLibrary::execK2_SetPointAttribute)
{
	P_GET_STRUCT(FVoxelPointHandle, Handle);
	P_GET_STRUCT(FName, Name);
	P_GET_STRUCT(FVoxelPinType, Type);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;

	Stack.StepCompiledIn<FProperty>(nullptr);

	const FProperty* Property = Stack.MostRecentProperty;
	const void* PropertyAddress = Stack.MostRecentPropertyAddress;

	P_FINISH;

	if (!ensure(Property))
	{
		VOXEL_MESSAGE(Error, "Failed to resolve the Value parameter");
		return;
	}

	P_NATIVE_BEGIN

	FString Error;
	if (!SetPointAttribute(
		Handle,
		Name,
		Type,
		FVoxelPinValue::MakeFromProperty(*Property, PropertyAddress),
		&Error))
	{
		VOXEL_MESSAGE(Error, "SetPointAttribute failed: {0}", Error);
	}

	P_NATIVE_END
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

DEFINE_FUNCTION(UVoxelPointBlueprintLibrary::execK2_GetPointAttribute)
{
	P_GET_STRUCT(FVoxelPointHandle, Handle);
	P_GET_STRUCT(FName, Name);
	P_GET_STRUCT(FVoxelPinType, InType);
	P_GET_ENUM_REF(EVoxelSuccess, Success);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;

	Stack.StepCompiledIn<FProperty>(nullptr);

	FProperty* Property = Stack.MostRecentProperty;
	void* PropertyAddress = Stack.MostRecentPropertyAddress;

	P_FINISH;

	if (!ensure(Property))
	{
		VOXEL_MESSAGE(Error, "Failed to resolve the Value parameter");
		return;
	}

	P_NATIVE_BEGIN

	FString Error;
	const FVoxelPinValue Value = GetPointAttribute(
		Handle,
		Name,
		&Error);

	if (!Value.IsValid())
	{
		VOXEL_MESSAGE(Error, "GetPointAttribute failed: {0}", Error);
		Success = EVoxelSuccess::Failed;
		return;
	}

	const FVoxelPinType Type(*Property);
	if (Value.GetType() != Type)
	{
		VOXEL_MESSAGE(Error, "Type mismatch: querying with type {0}, but attriute has type {1}", Type.ToString(), Value.GetType().ToString());
		Success = EVoxelSuccess::Failed;
		return;
	}

	Value.ExportToProperty(*Property, PropertyAddress);
	Success = EVoxelSuccess::Succeeded;

	P_NATIVE_END
}

FVoxelPinValue UVoxelPointBlueprintLibrary::GetPointAttribute(
	const FVoxelPointHandle& Handle,
	const FName Name,
	FString* OutError)
{
	VOXEL_FUNCTION_COUNTER();
	VOXEL_SCOPE_COUNTER_FNAME(Name);

	if (!Handle.IsValid())
	{
		if (OutError)
		{
			*OutError = "Invalid handle";
		}
		return {};
	}

	UWorld* World = Handle.GetWorld();
	if (!World)
	{
		if (OutError)
		{
			*OutError = "Invalid world";
		}
		return {};
	}

	const FVoxelRuntimePinValue CachedValue = FVoxelPointManager::Get(World)->FindAttribute(Name, Handle.ChunkRef, Handle.PointId);
	if (CachedValue.IsValid())
	{
		const FVoxelPinValue ExposedValue = FVoxelPinType::MakeExposedValue(CachedValue, false);
		ensure(ExposedValue.IsValid());
		return ExposedValue;
	}

	TVoxelMap<FName, FVoxelPinValue> Attributes;
	Attributes.Add_CheckNew(Name);

	if (!Handle.GetAttributes(Attributes, OutError))
	{
		return {};
	}

	const FVoxelPinValue Value = Attributes[Name];

	if (!Value.IsValid())
	{
		if (OutError)
		{
			TArray<FString> ValidNames;
			for (const auto& It : Attributes)
			{
				ValidNames.Add(It.Key.ToString());
			}

			*OutError = "No attribute named " + Name.ToString() + ". Valid names: " + FString::Join(ValidNames, TEXT(", "));
		}

		return {};
	}

	return Value;
}