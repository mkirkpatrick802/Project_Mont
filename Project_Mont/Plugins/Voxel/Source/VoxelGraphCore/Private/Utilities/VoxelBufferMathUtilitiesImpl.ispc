// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

export void VoxelBufferMathUtilities_Min8(
	const uniform float Buffer0[], const uniform bool bIsConstBuffer0,
	const uniform float Buffer1[], const uniform bool bIsConstBuffer1,
	const uniform float Buffer2[], const uniform bool bIsConstBuffer2,
	const uniform float Buffer3[], const uniform bool bIsConstBuffer3,
	const uniform float Buffer4[], const uniform bool bIsConstBuffer4,
	const uniform float Buffer5[], const uniform bool bIsConstBuffer5,
	const uniform float Buffer6[], const uniform bool bIsConstBuffer6,
	const uniform float Buffer7[], const uniform bool bIsConstBuffer7,
	uniform float Result[],
	const uniform int32 Num)
{
	if (!bIsConstBuffer0 &&
		!bIsConstBuffer1 &&
		!bIsConstBuffer2 &&
		!bIsConstBuffer3 &&
		!bIsConstBuffer4 &&
		!bIsConstBuffer5 &&
		!bIsConstBuffer6 &&
		!bIsConstBuffer7)
	{
		FOREACH(Index, 0, Num)
		{
			Result[Index] =
				min(Buffer0[Index],
				min(Buffer1[Index],
				min(Buffer2[Index],
				min(Buffer3[Index],
				min(Buffer4[Index],
				min(Buffer5[Index],
				min(Buffer6[Index], Buffer7[Index])))))));
		}
		return;
	}

	FOREACH(Index, 0, Num)
	{
		Result[Index] =
			min(bIsConstBuffer0 ? Buffer0[0] : Buffer0[Index],
			min(bIsConstBuffer1 ? Buffer1[0] : Buffer1[Index],
			min(bIsConstBuffer2 ? Buffer2[0] : Buffer2[Index],
			min(bIsConstBuffer3 ? Buffer3[0] : Buffer3[Index],
			min(bIsConstBuffer4 ? Buffer4[0] : Buffer4[Index],
			min(bIsConstBuffer5 ? Buffer5[0] : Buffer5[Index],
			min(bIsConstBuffer6 ? Buffer6[0] : Buffer6[Index], bIsConstBuffer7 ? Buffer7[0] : Buffer7[Index])))))));
	}
}

export void VoxelBufferMathUtilities_Max8(
	const uniform float Buffer0[], const uniform bool bIsConstBuffer0,
	const uniform float Buffer1[], const uniform bool bIsConstBuffer1,
	const uniform float Buffer2[], const uniform bool bIsConstBuffer2,
	const uniform float Buffer3[], const uniform bool bIsConstBuffer3,
	const uniform float Buffer4[], const uniform bool bIsConstBuffer4,
	const uniform float Buffer5[], const uniform bool bIsConstBuffer5,
	const uniform float Buffer6[], const uniform bool bIsConstBuffer6,
	const uniform float Buffer7[], const uniform bool bIsConstBuffer7,
	uniform float Result[],
	const uniform int32 Num)
{
	if (!bIsConstBuffer0 &&
		!bIsConstBuffer1 &&
		!bIsConstBuffer2 &&
		!bIsConstBuffer3 &&
		!bIsConstBuffer4 &&
		!bIsConstBuffer5 &&
		!bIsConstBuffer6 &&
		!bIsConstBuffer7)
	{
		FOREACH(Index, 0, Num)
		{
			Result[Index] =
				max(Buffer0[Index],
				max(Buffer1[Index],
				max(Buffer2[Index],
				max(Buffer3[Index],
				max(Buffer4[Index],
				max(Buffer5[Index],
				max(Buffer6[Index], Buffer7[Index])))))));
		}
		return;
	}

	FOREACH(Index, 0, Num)
	{
		Result[Index] =
			max(bIsConstBuffer0 ? Buffer0[0] : Buffer0[Index],
			max(bIsConstBuffer1 ? Buffer1[0] : Buffer1[Index],
			max(bIsConstBuffer2 ? Buffer2[0] : Buffer2[Index],
			max(bIsConstBuffer3 ? Buffer3[0] : Buffer3[Index],
			max(bIsConstBuffer4 ? Buffer4[0] : Buffer4[Index],
			max(bIsConstBuffer5 ? Buffer5[0] : Buffer5[Index],
			max(bIsConstBuffer6 ? Buffer6[0] : Buffer6[Index], bIsConstBuffer7 ? Buffer7[0] : Buffer7[Index])))))));
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void VoxelBufferMathUtilities_Add(
	const uniform float A[],
	const uniform bool bConstantA,
	const uniform float B[],
	const uniform bool bConstantB,
	uniform float Result[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Result[Index] = (bConstantA ? A[0] : A[Index]) + (bConstantB ? B[0] : B[Index]);
	}
}

export void VoxelBufferMathUtilities_Multiply(
	const uniform float A[],
	const uniform bool bConstantA,
	const uniform float B[],
	const uniform bool bConstantB,
	uniform float Result[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Result[Index] = (bConstantA ? A[0] : A[Index]) * (bConstantB ? B[0] : B[Index]);
	}
}

export void VoxelBufferMathUtilities_Less(
	const uniform float A[],
	const uniform bool bConstantA,
	const uniform float B[],
	const uniform bool bConstantB,
	uniform uint8 Result[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Result[Index] = (bConstantA ? A[0] : A[Index]) < (bConstantB ? B[0] : B[Index]) ? 1 : 0;
	}
}

export void VoxelBufferMathUtilities_Alpha(
	const uniform float A[],
	const uniform bool bConstantA,
	const uniform float B[],
	const uniform bool bConstantB,
	const uniform float Alpha[],
	const uniform bool bConstantAlpha,
	uniform float Result[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		Result[Index] = lerp(
			bConstantA ? A[0] : A[Index],
			bConstantB ? B[0] : B[Index],
			bConstantAlpha ? Alpha[0] : Alpha[Index]);
	}
}

export void VoxelBufferMathUtilities_Combine(
	const uniform float AX[],
	const uniform bool bConstantAX,
	const uniform float AY[],
	const uniform bool bConstantAY,
	const uniform float AZ[],
	const uniform bool bConstantAZ,
	const uniform float AW[],
	const uniform bool bConstantAW,
	const uniform float BX[],
	const uniform bool bConstantBX,
	const uniform float BY[],
	const uniform bool bConstantBY,
	const uniform float BZ[],
	const uniform bool bConstantBZ,
	const uniform float BW[],
	const uniform bool bConstantBW,
	uniform float X[],
	uniform float Y[],
	uniform float Z[],
	uniform float W[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const varying float A0 = bConstantAX ? AX[0] : AX[Index];
		const varying float A1 = bConstantAY ? AY[0] : AY[Index];
		const varying float A2 = bConstantAZ ? AZ[0] : AZ[Index];
		const varying float A3 = bConstantAW ? AW[0] : AW[Index];

		const varying float B0 = bConstantBX ? BX[0] : BX[Index];
		const varying float B1 = bConstantBY ? BY[0] : BY[Index];
		const varying float B2 = bConstantBZ ? BZ[0] : BZ[Index];
		const varying float B3 = bConstantBW ? BW[0] : BW[Index];

		X[Index] = A3 * B0 + A0 * B3 + A1 * B2 - A2 * B1;
		Y[Index] = A3 * B1 - A0 * B2 + A1 * B3 + A2 * B0;
		Z[Index] = A3 * B2 + A0 * B1 - A1 * B0 + A2 * B3;
		W[Index] = A3 * B3 - A0 * B0 - A1 * B1 - A2 * B2;
	}
}

export void VoxelBufferMathUtilities_MakeEulerFromQuaternion(
	const uniform float AX[],
	const uniform bool bConstantAX,
	const uniform float AY[],
	const uniform bool bConstantAY,
	const uniform float AZ[],
	const uniform bool bConstantAZ,
	const uniform float AW[],
	const uniform bool bConstantAW,
	uniform float X[],
	uniform float Y[],
	uniform float Z[],
	const uniform int32 Num)
{
	FOREACH(Index, 0, Num)
	{
		const varying float A0 = bConstantAX ? AX[0] : AX[Index];
		const varying float A1 = bConstantAY ? AY[0] : AY[Index];
		const varying float A2 = bConstantAZ ? AZ[0] : AZ[Index];
		const varying float A3 = bConstantAW ? AW[0] : AW[Index];
		const float4 Quat = MakeFloat4(A0, A1, A2, A3);

		float Pitch;
		float Yaw;
		float Roll;
		MakeEulerFromQuaternion(Quat, Pitch, Yaw, Roll);

		X[Index] = Pitch;
		Y[Index] = Yaw;
		Z[Index] = Roll;
	}
}