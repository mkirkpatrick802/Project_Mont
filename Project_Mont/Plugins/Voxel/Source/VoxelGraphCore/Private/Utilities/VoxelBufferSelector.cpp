// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "Utilities/VoxelBufferSelector.h"

TSharedRef<const FVoxelBuffer> FVoxelBufferSelector::SelectGeneric(
	const FVoxelPinType& InnerType,
	const FVoxelBuffer& Indices,
	const TConstVoxelArrayView<const FVoxelBuffer*> Buffers)
{
	if (Indices.IsA<FVoxelBoolBuffer>())
	{
		if (!ensure(Buffers.Num() == 2))
		{
			return FVoxelBuffer::Make(InnerType);
		}

		return Select(
			InnerType,
			CastChecked<FVoxelBoolBuffer>(Indices),
			*Buffers[0],
			*Buffers[1]);
	}
	if (Indices.IsA<FVoxelByteBuffer>())
	{
		return Select(
			InnerType,
			CastChecked<FVoxelByteBuffer>(Indices),
			Buffers);
	}
	if (Indices.IsA<FVoxelInt32Buffer>())
	{
		return Select(
			InnerType,
			CastChecked<FVoxelInt32Buffer>(Indices),
			Buffers);
	}

	ensure(false);
	return FVoxelBuffer::Make(InnerType);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TSharedRef<const FVoxelBuffer> FVoxelBufferSelector::Select(
	const FVoxelPinType& InnerType,
	const FVoxelBoolBuffer& Condition,
	const FVoxelBuffer& FalseBuffer,
	const FVoxelBuffer& TrueBuffer)
{
	VOXEL_FUNCTION_COUNTER();

	int32 Num = Condition.Num();
	if (!ensure(FalseBuffer.GetInnerType().CanBeCastedTo(InnerType)) ||
		!ensure(TrueBuffer.GetInnerType().CanBeCastedTo(InnerType)) ||
		!ensure(FVoxelBufferAccessor::MergeNum(Num, FalseBuffer)) ||
		!ensure(FVoxelBufferAccessor::MergeNum(Num, TrueBuffer)))
	{
		return FVoxelBuffer::Make(InnerType);
	}

	if (Condition.Num() == 0)
	{
		return FVoxelBuffer::MakeEmpty(InnerType);
	}
	if (Condition.IsConstant())
	{
		return (Condition.GetConstant() ? TrueBuffer : FalseBuffer).MakeSharedCopy();
	}

	const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::Make(InnerType);
	for (int32 Index = 0; Index < NewBuffer->NumTerminalBuffers(); Index++)
	{
		Select_Bool(
			NewBuffer->GetTerminalBuffer(Index),
			Condition,
			FalseBuffer.GetTerminalBuffer(Index),
			TrueBuffer.GetTerminalBuffer(Index));
	}
	return NewBuffer;
}

TSharedRef<const FVoxelBuffer> FVoxelBufferSelector::Select(
	const FVoxelPinType& InnerType,
	const FVoxelByteBuffer& Indices,
	const TConstVoxelArrayView<const FVoxelBuffer*> Buffers)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Check(InnerType, Indices, Buffers)))
	{
		return FVoxelBuffer::Make(InnerType);
	}

	if (Indices.Num() == 0)
	{
		return FVoxelBuffer::MakeEmpty(InnerType);
	}
	if (Indices.IsConstant())
	{
		const int32 Index = Indices.GetConstant();
		if (!Buffers.IsValidIndex(Index))
		{
			return FVoxelBuffer::Make(InnerType);
		}
		return Buffers[Index]->MakeSharedCopy();
	}

	const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::Make(InnerType);
	for (int32 Index = 0; Index < NewBuffer->NumTerminalBuffers(); Index++)
	{
		Select_Byte(
			NewBuffer->GetTerminalBuffer(Index),
			Indices,
			FVoxelBuffer::GetTerminalBuffers(Buffers, Index));
	}
	return NewBuffer;
}

TSharedRef<const FVoxelBuffer> FVoxelBufferSelector::Select(
	const FVoxelPinType& InnerType,
	const FVoxelInt32Buffer& Indices,
	const TConstVoxelArrayView<const FVoxelBuffer*> Buffers)
{
	VOXEL_FUNCTION_COUNTER();

	if (!ensure(Check(InnerType, Indices, Buffers)))
	{
		return FVoxelBuffer::Make(InnerType);
	}

	if (Indices.Num() == 0)
	{
		return FVoxelBuffer::MakeEmpty(InnerType);
	}
	if (Indices.IsConstant())
	{
		const int32 Index = Indices.GetConstant();
		if (!Buffers.IsValidIndex(Index))
		{
			return FVoxelBuffer::Make(InnerType);
		}
		return Buffers[Index]->MakeSharedCopy();
	}

	const TSharedRef<FVoxelBuffer> NewBuffer = FVoxelBuffer::Make(InnerType);
	for (int32 Index = 0; Index < NewBuffer->NumTerminalBuffers(); Index++)
	{
		Select_Int32(
			NewBuffer->GetTerminalBuffer(Index),
			Indices,
			FVoxelBuffer::GetTerminalBuffers(Buffers, Index));
	}
	return NewBuffer;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FVoxelBufferSelector::Check(
	const FVoxelPinType& InnerType,
	const FVoxelBuffer& Indices,
	const TConstVoxelArrayView<const FVoxelBuffer*> Buffers)
{
	if (!ensure(Buffers.Num() > 0))
	{
		return false;
	}

	int32 Num = Indices.Num();
	for (const FVoxelBuffer* Buffer : Buffers)
	{
		if (!ensure(Buffer->GetInnerType().CanBeCastedTo(InnerType)) ||
			!ensure(FVoxelBufferAccessor::MergeNum(Num, *Buffer)))
		{
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBufferSelector::Select_Bool(
	FVoxelTerminalBuffer& OutBuffer,
	const FVoxelBoolBuffer& Condition,
	const FVoxelBuffer& FalseBuffer,
	const FVoxelBuffer& TrueBuffer)
{
	VOXEL_FUNCTION_COUNTER_NUM(Condition.Num(), 128);
	check(Condition.Num() >= 2);

	if (OutBuffer.IsA<FVoxelComplexTerminalBuffer>())
	{
		FVoxelComplexTerminalBuffer& OutComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(OutBuffer);
		const FVoxelComplexTerminalBuffer& ComplexFalseBuffer = CastChecked<FVoxelComplexTerminalBuffer>(FalseBuffer);
		const FVoxelComplexTerminalBuffer& ComplexTrueBuffer = CastChecked<FVoxelComplexTerminalBuffer>(TrueBuffer);

		const TSharedRef<FVoxelComplexBufferStorage> Storage = OutComplexBuffer.MakeNewStorage();
		Storage->Allocate(Condition.Num());

		for (int32 Index = 0; Index < Condition.Num(); Index++)
		{
			(Condition[Index] ? ComplexTrueBuffer : ComplexFalseBuffer).GetStorage()[Index].CopyTo((*Storage)[Index]);
		}

		OutComplexBuffer.SetStorage(Storage);
		return;
	}

	FVoxelSimpleTerminalBuffer& OutSimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(OutBuffer);
	const FVoxelSimpleTerminalBuffer& SimpleFalseBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(FalseBuffer);
	const FVoxelSimpleTerminalBuffer& SimpleTrueBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(TrueBuffer);

	const TSharedRef<FVoxelBufferStorage> Storage = OutSimpleBuffer.MakeNewStorage();
	Storage->Allocate(Condition.Num());

	ForeachVoxelBufferChunk_Parallel(Condition.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		const TConstVoxelArrayView<bool> ConditionView = Condition.GetRawView_NotConstant(Iterator);

		VOXEL_SWITCH_TERMINAL_TYPE_SIZE(Storage->GetTypeSize())
		{
			using Type = VOXEL_GET_TYPE(TypeInstance);

			const TVoxelArrayView<Type> WriteView = Storage->As<Type>().GetRawView_NotConstant(Iterator);

			const TVoxelBufferStorage<Type>& FalseBufferStorage = SimpleFalseBuffer.GetStorage<Type>();
			const TVoxelBufferStorage<Type>& TrueBufferStorage = SimpleTrueBuffer.GetStorage<Type>();

			if (FalseBuffer.IsConstant() ||
				TrueBuffer.IsConstant())
			{
				for (int32 Index = 0; Index < Iterator.Num(); Index++)
				{
					WriteView[Index] = ConditionView[Index]
						? TrueBufferStorage[Iterator.GetIndex() + Index]
						: FalseBufferStorage[Iterator.GetIndex() + Index];
				}
			}
			else
			{
				const TConstVoxelArrayView<Type> FalseView = FalseBufferStorage.GetRawView_NotConstant(Iterator);
				const TConstVoxelArrayView<Type> TrueView = TrueBufferStorage.GetRawView_NotConstant(Iterator);

				for (int32 Index = 0; Index < Iterator.Num(); Index++)
				{
					WriteView[Index] = ConditionView[Index] ? TrueView[Index] : FalseView[Index];
				}
			}
		};
	});

	OutSimpleBuffer.SetStorage(Storage);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBufferSelector::Select_Byte(
	FVoxelTerminalBuffer& OutBuffer,
	const FVoxelByteBuffer& Indices,
	const TConstVoxelArrayView<const FVoxelTerminalBuffer*> Buffers)
{
	VOXEL_FUNCTION_COUNTER_NUM(Indices.Num(), 128);
	check(Indices.Num() >= 2);

	if (OutBuffer.IsA<FVoxelComplexTerminalBuffer>())
	{
		FVoxelComplexTerminalBuffer& OutComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(OutBuffer);
		const TConstVoxelArrayView<const FVoxelComplexTerminalBuffer*> ComplexBuffers = ReinterpretCastVoxelArrayView<const FVoxelComplexTerminalBuffer*>(Buffers);

		const TSharedRef<FVoxelComplexBufferStorage> Storage = OutComplexBuffer.MakeNewStorage();
		Storage->Allocate(Indices.Num());

		for (int32 Index = 0; Index < Indices.Num(); Index++)
		{
			const int32 BufferIndex = Indices[Index];
			if (!ComplexBuffers.IsValidIndex(BufferIndex))
			{
				continue;
			}

			ComplexBuffers[BufferIndex]->GetStorage()[Index].CopyTo((*Storage)[Index]);
		}

		OutComplexBuffer.SetStorage(Storage);
		return;
	}

	FVoxelSimpleTerminalBuffer& OutSimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(OutBuffer);
	const TConstVoxelArrayView<const FVoxelSimpleTerminalBuffer*> SimpleBuffers = ReinterpretCastVoxelArrayView<const FVoxelSimpleTerminalBuffer*>(Buffers);

	const TSharedRef<FVoxelBufferStorage> Storage = OutSimpleBuffer.MakeNewStorage();
	Storage->Allocate(Indices.Num());

	ForeachVoxelBufferChunk_Parallel(Indices.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		const TConstVoxelArrayView<uint8> IndicesView = Indices.GetRawView_NotConstant(Iterator);

		VOXEL_SWITCH_TERMINAL_TYPE_SIZE(Storage->GetTypeSize())
		{
			using Type = VOXEL_GET_TYPE(TypeInstance);
			const TVoxelArrayView<Type> WriteView = Storage->As<Type>().GetRawView_NotConstant(Iterator);

			for (int32 Index = 0; Index < Iterator.Num(); Index++)
			{
				const int32 BufferIndex = IndicesView[Index];

				if (!SimpleBuffers.IsValidIndex(BufferIndex))
				{
					WriteView[Index] = 0;
					continue;
				}

				const FVoxelSimpleTerminalBuffer* SimpleBuffer = SimpleBuffers[BufferIndex];
				WriteView[Index] = SimpleBuffer->GetStorage<Type>()[Iterator.GetIndex() + Index];
			}
		};
	});

	OutSimpleBuffer.SetStorage(Storage);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FVoxelBufferSelector::Select_Int32(
	FVoxelTerminalBuffer& OutBuffer,
	const FVoxelInt32Buffer& Indices,
	const TConstVoxelArrayView<const FVoxelTerminalBuffer*> Buffers)
{
	VOXEL_FUNCTION_COUNTER_NUM(Indices.Num(), 128);
	check(Indices.Num() >= 2);

	if (OutBuffer.IsA<FVoxelComplexTerminalBuffer>())
	{
		FVoxelComplexTerminalBuffer& OutComplexBuffer = CastChecked<FVoxelComplexTerminalBuffer>(OutBuffer);
		const TConstVoxelArrayView<const FVoxelComplexTerminalBuffer*> ComplexBuffers = ReinterpretCastVoxelArrayView<const FVoxelComplexTerminalBuffer*>(Buffers);

		const TSharedRef<FVoxelComplexBufferStorage> Storage = OutComplexBuffer.MakeNewStorage();
		Storage->Allocate(Indices.Num());

		for (int32 Index = 0; Index < Indices.Num(); Index++)
		{
			const int32 BufferIndex = Indices[Index];
			if (!ComplexBuffers.IsValidIndex(BufferIndex))
			{
				continue;
			}

			ComplexBuffers[BufferIndex]->GetStorage()[Index].CopyTo((*Storage)[Index]);
		}

		OutComplexBuffer.SetStorage(Storage);
		return;
	}

	FVoxelSimpleTerminalBuffer& OutSimpleBuffer = CastChecked<FVoxelSimpleTerminalBuffer>(OutBuffer);
	const TConstVoxelArrayView<const FVoxelSimpleTerminalBuffer*> SimpleBuffers = ReinterpretCastVoxelArrayView<const FVoxelSimpleTerminalBuffer*>(Buffers);

	const TSharedRef<FVoxelBufferStorage> Storage = OutSimpleBuffer.MakeNewStorage();
	Storage->Allocate(Indices.Num());

	ForeachVoxelBufferChunk_Parallel(Indices.Num(), [&](const FVoxelBufferIterator& Iterator)
	{
		const TConstVoxelArrayView<int32> IndicesView = Indices.GetRawView_NotConstant(Iterator);

		VOXEL_SWITCH_TERMINAL_TYPE_SIZE(Storage->GetTypeSize())
		{
			using Type = VOXEL_GET_TYPE(TypeInstance);
			const TVoxelArrayView<Type> WriteView = Storage->As<Type>().GetRawView_NotConstant(Iterator);

			for (int32 Index = 0; Index < Iterator.Num(); Index++)
			{
				const int32 BufferIndex = IndicesView[Index];

				if (!SimpleBuffers.IsValidIndex(BufferIndex))
				{
					WriteView[Index] = 0;
					continue;
				}

				const FVoxelSimpleTerminalBuffer* SimpleBuffer = SimpleBuffers[BufferIndex];
				WriteView[Index] = SimpleBuffer->GetStorage<Type>()[Iterator.GetIndex() + Index];
			}
		};
	});

	OutSimpleBuffer.SetStorage(Storage);
}