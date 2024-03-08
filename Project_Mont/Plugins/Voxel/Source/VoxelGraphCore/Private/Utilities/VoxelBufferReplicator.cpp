// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferReplicator.h"

TSharedRef<const FVoxelBuffer> FVoxelBufferReplicator::Replicate(
	const FVoxelBuffer& Buffer,
	const TConstVoxelArrayView<int32> Counts,
	const int32 NewNum)
{
	VOXEL_FUNCTION_COUNTER();

	if (VOXEL_DEBUG)
	{
		int32 NewNumCheck = 0;
		for (const int32 Count : Counts)
		{
			NewNumCheck += Count;
		}
		check(NewNum == NewNumCheck);
	}

	if (NewNum == 0)
	{
		return FVoxelBuffer::MakeEmpty(Buffer.GetInnerType());
	}
	if (Buffer.IsConstant())
	{
		return Buffer.MakeSharedCopy();
	}

	const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::Make(Buffer.GetInnerType());
	for (int32 Index = 0; Index < NewBuffer->NumTerminalBuffers(); Index++)
	{
		Replicate(
			NewBuffer->GetTerminalBuffer(Index),
			Buffer.GetTerminalBuffer(Index),
			Counts,
			NewNum);
	}
	return NewBuffer;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBufferReplicator::Replicate(
	FVoxelTerminalBuffer& OutBuffer,
	const FVoxelTerminalBuffer& Buffer,
	const TConstVoxelArrayView<int32> Counts,
	const int32 NewNum)
{
	VOXEL_FUNCTION_COUNTER_NUM(NewNum, 128);
	check(NewNum > 0);
	check(!Buffer.IsConstant());

	if (VOXEL_DEBUG)
	{
		int32 ActualNum = 0;
		for (const int32 Count : Counts)
		{
			ActualNum += Count;
		}
		check(NewNum == ActualNum);
	}

	if (Buffer.IsA<FVoxelComplexTerminalBuffer>())
	{
		FVoxelComplexTerminalBuffer& OutComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(OutBuffer);
		const FVoxelComplexTerminalBuffer& ComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(Buffer);

		const TSharedRef<FVoxelComplexBufferStorage> Storage = OutComplexBuffer.MakeNewStorage();
		Storage->Allocate(NewNum);

		int32 WriteIndex = 0;
		for (int32 ReadIndex = 0; ReadIndex < Counts.Num(); ReadIndex++)
		{
			const int32 Count = Counts[ReadIndex];
			const FConstVoxelStructView Value = ComplexBuffer.GetStorage()[ReadIndex];

			for (int32 Index = 0; Index < Count; Index++)
			{
				Value.CopyTo((*Storage)[WriteIndex++]);
			}
		}
		ensure(WriteIndex == NewNum);

		OutComplexBuffer.SetStorage(Storage);
		return;
	}

	FVoxelSimpleTerminalBuffer& OutSimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(OutBuffer);
	const FVoxelSimpleTerminalBuffer& SimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(Buffer);
	const int32 TypeSize = SimpleBuffer.GetTypeSize();

	const TSharedRef<FVoxelBufferStorage> Storage = OutSimpleBuffer.MakeNewStorage();
	Storage->Allocate(NewNum);

	VOXEL_SWITCH_TERMINAL_TYPE_SIZE(TypeSize)
	{
		using Type = VOXEL_GET_TYPE(TypeInstance);

		int32 WriteIndex = 0;
		for (int32 ReadIndex = 0; ReadIndex < Counts.Num(); ReadIndex++)
		{
			const int32 Count = Counts[ReadIndex];
			const Type Value = SimpleBuffer.GetStorage<Type>()[ReadIndex];

			FVoxelBufferIterator Iterator;
			Iterator.Initialize(WriteIndex + Count, WriteIndex);
			for (; Iterator; ++Iterator)
			{
				FVoxelUtilities::SetAll(
					Storage->As<Type>().GetRawView_NotConstant(Iterator),
					Value);
			}

			WriteIndex += Count;
		}
		ensure(WriteIndex == NewNum);
	};

	OutSimpleBuffer.SetStorage(Storage);
}