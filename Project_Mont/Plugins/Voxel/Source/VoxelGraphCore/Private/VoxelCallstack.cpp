// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelCallstack.h"
#include "VoxelGraph.h"
#include "VoxelMessageToken_Callstack.h"

uint32 FVoxelCallstack::GetHash() const
{
	VOXEL_FUNCTION_COUNTER();

	TVoxelInlineArray<uint32, 64> Hashes;
	for (const FVoxelCallstack* Callstack = this; Callstack; Callstack = Callstack->Parent.Get())
	{
		Hashes.Add(GetTypeHash(Callstack->GetPinName()));
		Hashes.Add(GetTypeHash(Callstack->GetNodeRef()));
	}
	return FVoxelUtilities::MurmurHashBytes(MakeByteVoxelArrayView(Hashes));
}

FString FVoxelCallstack::ToDebugString() const
{
	TStringBuilderWithBuffer<TCHAR, NAME_SIZE> Result;

	TVoxelInlineArray<const FVoxelCallstack*, 64> Callstacks;
	for (const FVoxelCallstack* Callstack = this; Callstack; Callstack = Callstack->Parent.Get())
	{
		Callstacks.Add(Callstack);
	}

	FVoxelGraphNodeRef LastNodeRef;
	for (int32 Index = Callstacks.Num() - 1; Index >= 0; Index--)
	{
		const FVoxelCallstack* Callstack = Callstacks[Index];
		if (Callstack->NodeRef.IsExplicitlyNull() ||
			Callstack->NodeRef == LastNodeRef)
		{
			continue;
		}
		LastNodeRef = Callstack->NodeRef;

		if (Callstack->NodeRef.EdGraphNodeTitle.IsNone())
		{
			ensure(!Callstack->NodeRef.NodeId.IsNone());
			Callstack->NodeRef.NodeId.AppendString(Result);
		}
		else
		{
			Callstack->NodeRef.EdGraphNodeTitle.AppendString(Result);
		}

		if (Index > 0)
		{
			Result += " -> ";
		}
	}

	return FString(Result.ToView());
}

TArray<FVoxelGraphNodeRef> FVoxelCallstack::Array() const
{
	TArray<FVoxelGraphNodeRef> Result;
	Result.Reset(32);

	for (const FVoxelCallstack* It = this; It; It = It->Parent.Get())
	{
		// Insert because we want the top callers to be first
		Result.Insert(It->GetNodeRef(), 0);
	}

	return Result;
}

TSharedRef<FVoxelMessageToken> FVoxelCallstack::CreateMessageToken() const
{
	const TSharedRef<FVoxelMessageToken_Callstack> Result = MakeVoxelShared<FVoxelMessageToken_Callstack>();
	Result->Callstack = AsShared();
	return Result;
}

TSharedRef<const FVoxelCallstack> FVoxelCallstack::MakeChild(
	const FName ChildPinName,
	const FVoxelGraphNodeRef& ChildNodeRef,
	const TWeakObjectPtr<UObject>& ChildRuntimeProvider) const
{
	const TSharedRef<FVoxelCallstack> Child = MakeVoxelShared<FVoxelCallstack>();
	Child->PinName = ChildPinName;
	Child->NodeRef = ChildNodeRef;
	Child->RuntimeProvider = ChildRuntimeProvider;
	Child->Parent = AsShared();
	return Child;
}