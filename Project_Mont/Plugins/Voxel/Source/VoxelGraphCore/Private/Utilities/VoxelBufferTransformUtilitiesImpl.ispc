// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

export void VoxelBufferTransformUtilities_ApplyTransform(
	const uniform float PositionX[], const uniform bool bConstPositionX,
	const uniform float PositionY[], const uniform bool bConstPositionY,
	const uniform float PositionZ[], const uniform bool bConstPositionZ,
	const uniform int32 Num,
	const uniform float3& Translation,
	const uniform float4& Rotation,
	const uniform float3& Scale,
	uniform float OutPositionsX[],
	uniform float OutPositionsY[],
	uniform float OutPositionsZ[])
{
	if (IsIdentity(Rotation))
	{
		if (bConstPositionX ||
			bConstPositionY ||
			bConstPositionZ)
		{
			FOREACH(Index, 0, Num)
			{
				const varying float3 OldPosition = MakeFloat3(
					bConstPositionX ? PositionX[0] : PositionX[Index],
					bConstPositionY ? PositionY[0] : PositionY[Index],
					bConstPositionZ ? PositionZ[0] : PositionZ[Index]);

				const varying float3 NewPosition = OldPosition * Scale + Translation;

				OutPositionsX[Index] = NewPosition.x;
				OutPositionsY[Index] = NewPosition.y;
				OutPositionsZ[Index] = NewPosition.z;
			}
		}
		else
		{
			FOREACH(Index, 0, Num)
			{
				const varying float3 OldPosition = MakeFloat3(
					PositionX[Index],
					PositionY[Index],
					PositionZ[Index]);

				const varying float3 NewPosition = OldPosition * Scale + Translation;

				OutPositionsX[Index] = NewPosition.x;
				OutPositionsY[Index] = NewPosition.y;
				OutPositionsZ[Index] = NewPosition.z;
			}
		}
	}
	else
	{
		if (bConstPositionX ||
			bConstPositionY ||
			bConstPositionZ)
		{
			FOREACH(Index, 0, Num)
			{
				const varying float3 OldPosition = MakeFloat3(
					bConstPositionX ? PositionX[0] : PositionX[Index],
					bConstPositionY ? PositionY[0] : PositionY[Index],
					bConstPositionZ ? PositionZ[0] : PositionZ[Index]);

				const varying float3 NewPosition = RotateVector(OldPosition * Scale, Rotation) + Translation;

				OutPositionsX[Index] = NewPosition.x;
				OutPositionsY[Index] = NewPosition.y;
				OutPositionsZ[Index] = NewPosition.z;
			}
		}
		else
		{
			FOREACH(Index, 0, Num)
			{
				const varying float3 OldPosition = MakeFloat3(
					PositionX[Index],
					PositionY[Index],
					PositionZ[Index]);

				const varying float3 NewPosition = RotateVector(OldPosition * Scale, Rotation) + Translation;

				OutPositionsX[Index] = NewPosition.x;
				OutPositionsY[Index] = NewPosition.y;
				OutPositionsZ[Index] = NewPosition.z;
			}
		}
	}
}

export void VoxelBufferTransformUtilities_ApplyInverseTransform(
	const uniform float PositionX[], const uniform bool bConstPositionX,
	const uniform float PositionY[], const uniform bool bConstPositionY,
	const uniform float PositionZ[], const uniform bool bConstPositionZ,
	const uniform int32 Num,
	const uniform float3& Translation,
	const uniform float4& Rotation,
	const uniform float3& Scale,
	uniform float OutPositionsX[],
	uniform float OutPositionsY[],
	uniform float OutPositionsZ[])
{
	uniform float3 InvScale;
	InvScale.x = Scale.x != 0 ? 1.f / Scale.x : 1.f;
	InvScale.y = Scale.y != 0 ? 1.f / Scale.y : 1.f;
	InvScale.z = Scale.z != 0 ? 1.f / Scale.z : 1.f;

	if (IsIdentity(Rotation))
	{
		FOREACH(Index, 0, Num)
		{
			const varying float3 OldPosition = MakeFloat3(
				bConstPositionX ? PositionX[0] : PositionX[Index],
				bConstPositionY ? PositionY[0] : PositionY[Index],
				bConstPositionZ ? PositionZ[0] : PositionZ[Index]);

			const varying float3 NewPosition = (OldPosition - Translation) * InvScale;

			OutPositionsX[Index] = NewPosition.x;
			OutPositionsY[Index] = NewPosition.y;
			OutPositionsZ[Index] = NewPosition.z;
		}
	}
	else
	{
		FOREACH(Index, 0, Num)
		{
			const varying float3 OldPosition = MakeFloat3(
				bConstPositionX ? PositionX[0] : PositionX[Index],
				bConstPositionY ? PositionY[0] : PositionY[Index],
				bConstPositionZ ? PositionZ[0] : PositionZ[Index]);

			const varying float3 NewPosition = UnrotateVector(OldPosition - Translation, Rotation) * InvScale;

			OutPositionsX[Index] = NewPosition.x;
			OutPositionsY[Index] = NewPosition.y;
			OutPositionsZ[Index] = NewPosition.z;
		}
	}
}

export void VoxelBufferTransformUtilities_ApplyTransform_Bulk(
	const uniform float PositionX[], const uniform bool bConstPositionX,
	const uniform float PositionY[], const uniform bool bConstPositionY,
	const uniform float PositionZ[], const uniform bool bConstPositionZ,
	const uniform float TranslationX[], const uniform bool bConstTranslationX,
	const uniform float TranslationY[], const uniform bool bConstTranslationY,
	const uniform float TranslationZ[], const uniform bool bConstTranslationZ,
	const uniform float RotationX[], const uniform bool bConstRotationX,
	const uniform float RotationY[], const uniform bool bConstRotationY,
	const uniform float RotationZ[], const uniform bool bConstRotationZ,
	const uniform float RotationW[], const uniform bool bConstRotationW,
	const uniform float ScaleX[], const uniform bool bConstScaleX,
	const uniform float ScaleY[], const uniform bool bConstScaleY,
	const uniform float ScaleZ[], const uniform bool bConstScaleZ,
	const uniform int32 Num,
	uniform float OutPositionsX[],
	uniform float OutPositionsY[],
	uniform float OutPositionsZ[])
{
	FOREACH(Index, 0, Num)
	{
		const varying float3 OldPosition = MakeFloat3(
			bConstPositionX ? PositionX[0] : PositionX[Index],
			bConstPositionY ? PositionY[0] : PositionY[Index],
			bConstPositionZ ? PositionZ[0] : PositionZ[Index]);

		const varying float3 Translation = MakeFloat3(
			bConstTranslationX ? TranslationX[0] : TranslationX[Index],
			bConstTranslationY ? TranslationY[0] : TranslationY[Index],
			bConstTranslationZ ? TranslationZ[0] : TranslationZ[Index]);

		const varying float4 Rotation = MakeFloat4(
			bConstRotationX ? RotationX[0] : RotationX[Index],
			bConstRotationY ? RotationY[0] : RotationY[Index],
			bConstRotationZ ? RotationZ[0] : RotationZ[Index],
			bConstRotationW ? RotationW[0] : RotationW[Index]);

		const varying float3 Scale = MakeFloat3(
			bConstScaleX ? ScaleX[0] : ScaleX[Index],
			bConstScaleY ? ScaleY[0] : ScaleY[Index],
			bConstScaleZ ? ScaleZ[0] : ScaleZ[Index]);

		const varying float3 NewPosition = RotateVector(OldPosition * Scale, Rotation) + Translation;

		OutPositionsX[Index] = NewPosition.x;
		OutPositionsY[Index] = NewPosition.y;
		OutPositionsZ[Index] = NewPosition.z;
	}
}

export void VoxelBufferTransformUtilities_ApplyTransform_Matrix(
	const uniform float PositionX[], const uniform bool bConstPositionX,
	const uniform float PositionY[], const uniform bool bConstPositionY,
	const uniform float PositionZ[], const uniform bool bConstPositionZ,
	const uniform int32 Num,
	const uniform float3& Translation,
	const uniform float3& MatrixX,
	const uniform float3& MatrixY,
	const uniform float3& MatrixZ,
	uniform float OutPositionsX[],
	uniform float OutPositionsY[],
	uniform float OutPositionsZ[])
{
	if (bConstPositionX ||
		bConstPositionY ||
		bConstPositionZ)
	{
		FOREACH(Index, 0, Num)
		{
			const varying float3 OldPosition = MakeFloat3(
				bConstPositionX ? PositionX[0] : PositionX[Index],
				bConstPositionY ? PositionY[0] : PositionY[Index],
				bConstPositionZ ? PositionZ[0] : PositionZ[Index]);

			const varying float3 NewPosition =
				Translation +
				MatrixX * OldPosition.x +
				MatrixY * OldPosition.y +
				MatrixZ * OldPosition.z;

			OutPositionsX[Index] = NewPosition.x;
			OutPositionsY[Index] = NewPosition.y;
			OutPositionsZ[Index] = NewPosition.z;
		}
	}
	else
	{
		FOREACH(Index, 0, Num)
		{
			const varying float3 OldPosition = MakeFloat3(
				PositionX[Index],
				PositionY[Index],
				PositionZ[Index]);

			const varying float3 NewPosition =
				Translation +
				MatrixX * OldPosition.x +
				MatrixY * OldPosition.y +
				MatrixZ * OldPosition.z;

			OutPositionsX[Index] = NewPosition.x;
			OutPositionsY[Index] = NewPosition.y;
			OutPositionsZ[Index] = NewPosition.z;
		}
	}
}

export void VoxelBufferTransformUtilities_TransformDistance(
	const uniform float Distance[],
	uniform float OutDistance[],
	const uniform float Scale,
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		OutDistance[Index] = Distance[Index] * Scale;
	}
}