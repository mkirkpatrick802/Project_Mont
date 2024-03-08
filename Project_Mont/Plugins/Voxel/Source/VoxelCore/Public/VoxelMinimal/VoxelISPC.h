// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelCoreMinimal.h"
#include "Math/TransformCalculus2D.h"

#define __ISPC_STRUCT_float2__
#define __ISPC_STRUCT_float3__
#define __ISPC_STRUCT_float4__

#define __ISPC_STRUCT_double2__
#define __ISPC_STRUCT_double3__
#define __ISPC_STRUCT_double4__

#define __ISPC_STRUCT_int2__
#define __ISPC_STRUCT_int3__
#define __ISPC_STRUCT_int4__

#define __ISPC_STRUCT_double2x2__
#define __ISPC_STRUCT_FTransform2d__
#define __ISPC_STRUCT_float4x4__
#define __ISPC_STRUCT_FColor__

namespace ispc
{
	struct float2
	{
		float X;
		float Y;
	};
	struct float3
	{
		float X;
		float Y;
		float Z;
	};
	struct float4
	{
		float X;
		float Y;
		float Z;
		float W;
	};

	struct double2
	{
		double X;
		double Y;
	};
	struct double3
	{
		double X;
		double Y;
		double Z;
	};
	struct double4
	{
		double X;
		double Y;
		double Z;
		double W;
	};

	struct int2
	{
		int32 X;
		int32 Y;
	};
	struct int3
	{
		int32 X;
		int32 Y;
		int32 Z;
	};
	struct int4
	{
		int32 X;
		int32 Y;
		int32 Z;
		int32 W;
	};

	struct double2x2
	{
		double M[2][2];
	};

	struct FTransform2d
	{
		double2x2 Matrix;
		double2 Translation;
	};

	struct float4x4
	{
		float M[16];
	};

	struct FColor
	{
		uint8 B;
		uint8 G;
		uint8 R;
		uint8 A;
	};
}

FORCEINLINE ispc::float2 GetISPCValue(const FVector2f& Vector)
{
	return ReinterpretCastRef<ispc::float2>(Vector);
}
FORCEINLINE ispc::float3 GetISPCValue(const FVector3f& Vector)
{
	return ReinterpretCastRef<ispc::float3>(Vector);
}
FORCEINLINE ispc::float4 GetISPCValue(const FVector4f& Vector)
{
	return ReinterpretCastRef<ispc::float4>(Vector);
}

FORCEINLINE ispc::double2 GetISPCValue(const FVector2d& Vector)
{
	return ReinterpretCastRef<ispc::double2>(Vector);
}
FORCEINLINE ispc::double3 GetISPCValue(const FVector3d& Vector)
{
	return ReinterpretCastRef<ispc::double3>(Vector);
}
FORCEINLINE ispc::double4 GetISPCValue(const FVector4d& Vector)
{
	return ReinterpretCastRef<ispc::double4>(Vector);
}

FORCEINLINE ispc::int2 GetISPCValue(const FIntPoint& Vector)
{
	return ReinterpretCastRef<ispc::int2>(Vector);
}
FORCEINLINE ispc::int3 GetISPCValue(const FIntVector& Vector)
{
	return ReinterpretCastRef<ispc::int3>(Vector);
}
FORCEINLINE ispc::int4 GetISPCValue(const FIntVector4& Vector)
{
	return ReinterpretCastRef<ispc::int4>(Vector);
}

FORCEINLINE ispc::double2x2 GetISPCValue(const FMatrix2x2d& Matrix)
{
	return ReinterpretCastRef<ispc::double2x2>(Matrix);
}

FORCEINLINE ispc::FTransform2d GetISPCValue(const FTransform2d& Transform)
{
	return ReinterpretCastRef<ispc::FTransform2d>(Transform);
}

FORCEINLINE ispc::float4x4 GetISPCValue(const FMatrix44f& Matrix)
{
	return ReinterpretCastRef<ispc::float4x4>(Matrix);
}

FORCEINLINE ispc::FColor GetISPCValue(const FColor& Color)
{
	return ReinterpretCastRef<ispc::FColor>(Color);
}