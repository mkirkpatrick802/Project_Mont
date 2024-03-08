// Copyright Voxel Plugin SAS. All Rights Reserved.

#include "VoxelMinimal.isph"

// ReSharper disable CppCStyleCast

// X+
// U = Y
// V = -Z

// X-
// U = Y
// V = Z

// Y+
// U = -X
// V = -Z

// Y-
// U = -X
// V = Z

// Z+
// U = Y
// V = X

// Z-
// U = -Y
// V = X

export void VoxelCubemapFunctionLibrary_MakeCubemapPlanetDistanceField_Bilinear(
	const uniform float ArrayPositionX[],
	const uniform bool bConstPositionX,
	const uniform float ArrayPositionY[],
	const uniform bool bConstPositionY,
	const uniform float ArrayPositionZ[],
	const uniform bool bConstPositionZ,
	const uniform double3& PlanetCenter,
	const uniform double PlanetRadius,
	const uniform double MaxHeight,
	const uniform int32 SizeX,
	const uniform int32 SizeY,
	const uniform uint16* const uniform Heightmaps[6],
	const uniform int32 Num,
	uniform float Distances[])
{
	const uniform float HalfSizeX = SizeX / 2.f;
	const uniform float HalfSizeY = SizeY / 2.f;
	const uniform double MaxHeightDivided = MaxHeight / MAX_uint16;

	const uniform float SizeXMinus2 = SizeX - 2.f;
	const uniform float SizeYMinus2 = SizeY - 2.f;

	FOREACH(Index, 0, Num)
	{
		const varying double FullX = (bConstPositionX ? ArrayPositionX[0] : ArrayPositionX[Index]) - PlanetCenter.x;
		const varying double FullY = (bConstPositionY ? ArrayPositionY[0] : ArrayPositionY[Index]) - PlanetCenter.y;
		const varying double FullZ = (bConstPositionZ ? ArrayPositionZ[0] : ArrayPositionZ[Index]) - PlanetCenter.z;

		const varying double Radius = length(MakeDouble3(FullX, FullY, FullZ));
		const varying double InvRadius = 1. / Radius;

		const varying float X = FullX * InvRadius;
		const varying float Y = FullY * InvRadius;
		const varying float Z = FullZ * InvRadius;

		const varying float AbsX = abs(X);
		const varying float AbsY = abs(Y);
		const varying float AbsZ = abs(Z);

		varying int32 FaceToReduce =
			AbsX >= AbsY &&
			AbsX >= AbsZ
			? 0
			: AbsY >= AbsZ
			? 2
			: 4;

		const varying float FaceValue =
			FaceToReduce == 0
			? X
			: FaceToReduce == 2
			? Y
			: Z;

		FaceToReduce += FaceValue > 0 ? 0 : 1;

		varying float SampledHeight;
		uniform int32 UniformFace;
		if (reduce_equal(FaceToReduce, &UniformFace))
		{
			const uniform int32 Face = UniformFace;

			varying float U =
				Face == 2 || Face == 3
				? -X
				: Face == 5
				? -Y
				: Y;

			varying float V =
				Face == 0 || Face == 2
				? -Z
				: Face == 1 || Face == 3
				? Z
				: X;

			const varying float InvFaceValue = 1. / FaceValue;

			U *= InvFaceValue;
			V *= InvFaceValue;

			const varying float SampleX = (U + 1.f) * HalfSizeX;
			const varying float SampleY = (V + 1.f) * HalfSizeY;

			const varying float MinXf = clamp(floor(SampleX), 0.f, SizeXMinus2);
			const varying float MinYf = clamp(floor(SampleY), 0.f, SizeYMinus2);

			const varying float AlphaX = clamp(SampleX - MinXf, 0.f, 1.f);
			const varying float AlphaY = clamp(SampleY - MinYf, 0.f, 1.f);

			const varying int32 MinX = (int32)MinXf;
			const varying int32 MinY = (int32)MinYf;
			const varying int32 MaxX = MinX + 1;
			const varying int32 MaxY = MinY + 1;

			const uniform uint16* uniform Heightmap = Heightmaps[UniformFace];

			SampledHeight = BilinearInterpolation(
				IGNORE_PERF_WARNING
				Heightmap[MinX + SizeX * MinY],
				IGNORE_PERF_WARNING
				Heightmap[MaxX + SizeX * MinY],
				IGNORE_PERF_WARNING
				Heightmap[MinX + SizeX * MaxY],
				IGNORE_PERF_WARNING
				Heightmap[MaxX + SizeX * MaxY],
				AlphaX,
				AlphaY);
		}
		else
		{
			const varying int32 Face = FaceToReduce;

			varying float U =
				Face == 2 || Face == 3
				? -X
				: Face == 5
				? -Y
				: Y;

			varying float V =
				Face == 0 || Face == 2
				? -Z
				: Face == 1 || Face == 3
				? Z
				: X;

			const varying float InvFaceValue = 1. / FaceValue;

			U *= InvFaceValue;
			V *= InvFaceValue;

			const varying float SampleX = (U + 1.f) * HalfSizeX;
			const varying float SampleY = (V + 1.f) * HalfSizeY;

			const varying float MinXf = clamp(floor(SampleX), 0.f, SizeXMinus2);
			const varying float MinYf = clamp(floor(SampleY), 0.f, SizeYMinus2);

			const varying float AlphaX = clamp(SampleX - MinXf, 0.f, 1.f);
			const varying float AlphaY = clamp(SampleY - MinYf, 0.f, 1.f);

			const varying int32 MinX = (int32)MinXf;
			const varying int32 MinY = (int32)MinYf;
			const varying int32 MaxX = MinX + 1;
			const varying int32 MaxY = MinY + 1;

			IGNORE_PERF_WARNING
			const uniform uint16* varying Heightmap = Heightmaps[Face];

			SampledHeight = BilinearInterpolation(
				IGNORE_PERF_WARNING
				Heightmap[MinX + SizeX * MinY],
				IGNORE_PERF_WARNING
				Heightmap[MaxX + SizeX * MinY],
				IGNORE_PERF_WARNING
				Heightmap[MinX + SizeX * MaxY],
				IGNORE_PERF_WARNING
				Heightmap[MaxX + SizeX * MaxY],
				AlphaX,
				AlphaY);
		}

		const varying double Height = PlanetRadius + SampledHeight * MaxHeightDivided;

		Distances[Index] = Radius - Height;
	}
}

export void VoxelCubemapFunctionLibrary_MakeCubemapPlanetDistanceField_Bicubic(
	const uniform float ArrayPositionX[],
	const uniform bool bConstPositionX,
	const uniform float ArrayPositionY[],
	const uniform bool bConstPositionY,
	const uniform float ArrayPositionZ[],
	const uniform bool bConstPositionZ,
	const uniform double3& PlanetCenter,
	const uniform double PlanetRadius,
	const uniform double MaxHeight,
	const uniform float Smoothness,
	const uniform int32 SizeX,
	const uniform int32 SizeY,
	const uniform uint16* const uniform Heightmaps[6],
	const uniform int32 Num,
	uniform float Distances[])
{
	const uniform float HalfSizeX = SizeX / 2.f;
	const uniform float HalfSizeY = SizeY / 2.f;
	const uniform double MaxHeightDivided = MaxHeight / MAX_uint16;

	const uniform float SizeXMinus2 = SizeX - 2.f;
	const uniform float SizeYMinus2 = SizeY - 2.f;

	uniform float p0A, p0B, p0C, p0D;
	uniform float p1A, p1B, p1C, p1D;
	uniform float p2A, p2B, p2C, p2D;
	uniform float p3A, p3B, p3C, p3D;
	{
		const uniform float B = clamp(Smoothness, 0.f, 1.f);
		const uniform float C = (1.f - B) / 2.f;

		// See https://en.wikipedia.org/wiki/Mitchell%E2%80%93Netravali_filters
		p0A = B / 6.f;       p0B = -B / 2.f - C; p0C = B / 2.f + 2.f * C;              p0D = -B / 6.f - C;
		p1A = 1.f - B / 3.f; p1B = 0.f;          p1C = 2.f * B + C - 3.f;              p1D = -3.f / 2.f * B - C + 2.f;
		p2A = B / 6.f;       p2B = +B / 2.f + C; p2C = -5.f / 2.f * B - 2.f * C + 3.f; p2D = +3.f / 2.f * B + C - 2.f;
		p3A = 0.f;           p3B = 0.f;          p3C = -C;                             p3D = +B / 6.f + C;
	}

	FOREACH(Index, 0, Num)
	{
		const varying double FullX = (bConstPositionX ? ArrayPositionX[0] : ArrayPositionX[Index]) - PlanetCenter.x;
		const varying double FullY = (bConstPositionY ? ArrayPositionY[0] : ArrayPositionY[Index]) - PlanetCenter.y;
		const varying double FullZ = (bConstPositionZ ? ArrayPositionZ[0] : ArrayPositionZ[Index]) - PlanetCenter.z;

		const varying double Radius = length(MakeDouble3(FullX, FullY, FullZ));
		const varying double InvRadius = 1. / Radius;

		const varying float X = FullX * InvRadius;
		const varying float Y = FullY * InvRadius;
		const varying float Z = FullZ * InvRadius;

		const varying float AbsX = abs(X);
		const varying float AbsY = abs(Y);
		const varying float AbsZ = abs(Z);

		varying int32 FaceToReduce =
			AbsX >= AbsY &&
			AbsX >= AbsZ
			? 0
			: AbsY >= AbsZ
			? 2
			: 4;

		const varying float FaceValue =
			FaceToReduce == 0
			? X
			: FaceToReduce == 2
			? Y
			: Z;

		FaceToReduce += FaceValue > 0 ? 0 : 1;

		varying float SampledHeight;
		uniform int32 UniformFace;
		if (reduce_equal(FaceToReduce, &UniformFace))
		{
			const uniform int32 Face = UniformFace;

			varying float U =
				Face == 2 || Face == 3
				? -X
				: Face == 5
				? -Y
				: Y;

			varying float V =
				Face == 0 || Face == 2
				? -Z
				: Face == 1 || Face == 3
				? Z
				: X;

			const varying float InvFaceValue = 1. / FaceValue;

			U *= InvFaceValue;
			V *= InvFaceValue;

			const varying float SampleX = (U + 1.f) * HalfSizeX;
			const varying float SampleY = (V + 1.f) * HalfSizeY;

			const varying float MinXf = clamp(floor(SampleX), 0.f, SizeXMinus2);
			const varying float MinYf = clamp(floor(SampleY), 0.f, SizeYMinus2);

			const varying float AlphaX = clamp(SampleX - MinXf, 0.f, 1.f);
			const varying float AlphaY = clamp(SampleY - MinYf, 0.f, 1.f);

			const varying int32 MinX = (int32)MinXf;
			const varying int32 MinY = (int32)MinYf;

			IGNORE_PERF_WARNING
			const uniform uint16* varying Heightmap = Heightmaps[Face];

			IGNORE_PERF_WARNING
			const varying float p00 = Heightmap[(MinX - 1) + SizeX * (MinY - 1)];
			IGNORE_PERF_WARNING
			const varying float p01 = Heightmap[(MinX + 0) + SizeX * (MinY - 1)];
			IGNORE_PERF_WARNING
			const varying float p02 = Heightmap[(MinX + 1) + SizeX * (MinY - 1)];
			IGNORE_PERF_WARNING
			const varying float p03 = Heightmap[(MinX + 2) + SizeX * (MinY - 1)];

			IGNORE_PERF_WARNING
			const varying float p10 = Heightmap[(MinX - 1) + SizeX * (MinY + 0)];
			IGNORE_PERF_WARNING
			const varying float p11 = Heightmap[(MinX + 0) + SizeX * (MinY + 0)];
			IGNORE_PERF_WARNING
			const varying float p12 = Heightmap[(MinX + 1) + SizeX * (MinY + 0)];
			IGNORE_PERF_WARNING
			const varying float p13 = Heightmap[(MinX + 2) + SizeX * (MinY + 0)];

			IGNORE_PERF_WARNING
			const varying float p20 = Heightmap[(MinX - 1) + SizeX * (MinY + 1)];
			IGNORE_PERF_WARNING
			const varying float p21 = Heightmap[(MinX + 0) + SizeX * (MinY + 1)];
			IGNORE_PERF_WARNING
			const varying float p22 = Heightmap[(MinX + 1) + SizeX * (MinY + 1)];
			IGNORE_PERF_WARNING
			const varying float p23 = Heightmap[(MinX + 2) + SizeX * (MinY + 1)];

			IGNORE_PERF_WARNING
			const varying float p30 = Heightmap[(MinX - 1) + SizeX * (MinY + 2)];
			IGNORE_PERF_WARNING
			const varying float p31 = Heightmap[(MinX + 0) + SizeX * (MinY + 2)];
			IGNORE_PERF_WARNING
			const varying float p32 = Heightmap[(MinX + 1) + SizeX * (MinY + 2)];
			IGNORE_PERF_WARNING
			const varying float p33 = Heightmap[(MinX + 2) + SizeX * (MinY + 2)];

			const varying float p0 =
				p00 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p01 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p02 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p03 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			const varying float p1 =
				p10 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p11 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p12 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p13 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			const varying float p2 =
				p20 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p21 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p22 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p23 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			const varying float p3 =
				p30 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p31 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p32 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p33 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			SampledHeight =
				p0 * (p0A + AlphaY * (p0B + AlphaY * (p0C + AlphaY * p0D))) +
				p1 * (p1A + AlphaY * (p1B + AlphaY * (p1C + AlphaY * p1D))) +
				p2 * (p2A + AlphaY * (p2B + AlphaY * (p2C + AlphaY * p2D))) +
				p3 * (p3A + AlphaY * (p3B + AlphaY * (p3C + AlphaY * p3D)));
		}
		else
		{
			const varying int32 Face = FaceToReduce;

			varying float U =
				Face == 2 || Face == 3
				? -X
				: Face == 5
				? -Y
				: Y;

			varying float V =
				Face == 0 || Face == 2
				? -Z
				: Face == 1 || Face == 3
				? Z
				: X;

			const varying float InvFaceValue = 1. / FaceValue;

			U *= InvFaceValue;
			V *= InvFaceValue;

			const varying float SampleX = (U + 1.f) * HalfSizeX;
			const varying float SampleY = (V + 1.f) * HalfSizeY;

			const varying float MinXf = clamp(floor(SampleX), 0.f, SizeXMinus2);
			const varying float MinYf = clamp(floor(SampleY), 0.f, SizeYMinus2);

			const varying float AlphaX = clamp(SampleX - MinXf, 0.f, 1.f);
			const varying float AlphaY = clamp(SampleY - MinYf, 0.f, 1.f);

			const varying int32 MinX = (int32)MinXf;
			const varying int32 MinY = (int32)MinYf;

			IGNORE_PERF_WARNING
			const uniform uint16* varying Heightmap = Heightmaps[Face];

			IGNORE_PERF_WARNING
			const varying float p00 = Heightmap[(MinX - 1) + SizeX * (MinY - 1)];
			IGNORE_PERF_WARNING
			const varying float p01 = Heightmap[(MinX + 0) + SizeX * (MinY - 1)];
			IGNORE_PERF_WARNING
			const varying float p02 = Heightmap[(MinX + 1) + SizeX * (MinY - 1)];
			IGNORE_PERF_WARNING
			const varying float p03 = Heightmap[(MinX + 2) + SizeX * (MinY - 1)];

			IGNORE_PERF_WARNING
			const varying float p10 = Heightmap[(MinX - 1) + SizeX * (MinY + 0)];
			IGNORE_PERF_WARNING
			const varying float p11 = Heightmap[(MinX + 0) + SizeX * (MinY + 0)];
			IGNORE_PERF_WARNING
			const varying float p12 = Heightmap[(MinX + 1) + SizeX * (MinY + 0)];
			IGNORE_PERF_WARNING
			const varying float p13 = Heightmap[(MinX + 2) + SizeX * (MinY + 0)];

			IGNORE_PERF_WARNING
			const varying float p20 = Heightmap[(MinX - 1) + SizeX * (MinY + 1)];
			IGNORE_PERF_WARNING
			const varying float p21 = Heightmap[(MinX + 0) + SizeX * (MinY + 1)];
			IGNORE_PERF_WARNING
			const varying float p22 = Heightmap[(MinX + 1) + SizeX * (MinY + 1)];
			IGNORE_PERF_WARNING
			const varying float p23 = Heightmap[(MinX + 2) + SizeX * (MinY + 1)];

			IGNORE_PERF_WARNING
			const varying float p30 = Heightmap[(MinX - 1) + SizeX * (MinY + 2)];
			IGNORE_PERF_WARNING
			const varying float p31 = Heightmap[(MinX + 0) + SizeX * (MinY + 2)];
			IGNORE_PERF_WARNING
			const varying float p32 = Heightmap[(MinX + 1) + SizeX * (MinY + 2)];
			IGNORE_PERF_WARNING
			const varying float p33 = Heightmap[(MinX + 2) + SizeX * (MinY + 2)];

			const varying float p0 =
				p00 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p01 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p02 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p03 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			const varying float p1 =
				p10 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p11 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p12 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p13 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			const varying float p2 =
				p20 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p21 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p22 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p23 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			const varying float p3 =
				p30 * (p0A + AlphaX * (p0B + AlphaX * (p0C + AlphaX * p0D))) +
				p31 * (p1A + AlphaX * (p1B + AlphaX * (p1C + AlphaX * p1D))) +
				p32 * (p2A + AlphaX * (p2B + AlphaX * (p2C + AlphaX * p2D))) +
				p33 * (p3A + AlphaX * (p3B + AlphaX * (p3C + AlphaX * p3D)));

			SampledHeight =
				p0 * (p0A + AlphaY * (p0B + AlphaY * (p0C + AlphaY * p0D))) +
				p1 * (p1A + AlphaY * (p1B + AlphaY * (p1C + AlphaY * p1D))) +
				p2 * (p2A + AlphaY * (p2B + AlphaY * (p2C + AlphaY * p2D))) +
				p3 * (p3A + AlphaY * (p3B + AlphaY * (p3C + AlphaY * p3D)));
		}

		const varying double Height = PlanetRadius + SampledHeight * MaxHeightDivided;

		Distances[Index] = Radius - Height;
	}
}