// Copyright Voxel Plugin SAS. All Rights Reserved.

#pragma once

#include "VoxelMinimal.h"
#include "VoxelizedMesh/VoxelVoxelizedMeshSettings.h"

struct FVoxelMeshVoxelizer
{
public:
	static void Voxelize(
		const FVoxelVoxelizedMeshSettings& Settings,
		const TVoxelArray<FVector3f>& Vertices,
		const TVoxelArray<int32>& Indices,
		const FVector3f& Origin,
		const FIntVector& Size,
		TVoxelArray<float>& Distances,
		TVoxelArray<FVector3f>& Positions,
		const TVoxelArray<FVector3f>* VertexNormals = nullptr,
		TVoxelArray<FVector3f>* VoxelNormals = nullptr,
		int32* OutNumLeaks = nullptr);

private:
	// Calculate twice signed area of triangle (0,0)-(A.X,A.Y)-(B.X,B.Y)
	// Return an SOS-determined sign (-1, +1, or 0 only if it's a truly degenerate triangle)
	static int32 Orientation(
		const FVector2d& A,
		const FVector2d& B,
		double& TwiceSignedArea);

	// Robust test of (x0,y0) in the triangle (x1,y1)-(x2,y2)-(x3,y3)
	// If true is returned, the barycentric coordinates are set in a,b,c.
	static bool PointInTriangle2D(
		const FVector2d& Point,
		FVector2d A,
		FVector2d B,
		FVector2d C,
		double& AlphaA, double& AlphaB, double& AlphaC);

	static void GetTriangleBarycentrics(
		const FVector3f& P,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		float& OutAlphaA,
		float& OutAlphaB,
		float& OutAlphaC);

	static float PointSegmentDistanceSquared(
		const FVector3f& P,
		const FVector3f& A,
		const FVector3f& B,
		float& OutAlpha);
	static float PointSegmentDistanceSquared(
		const FVector3f& P,
		const FVector3f& A,
		const FVector3f& B);

	static float PointTriangleDistanceSquared(
		const FVector3f& P,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C);

	static float PointTriangleDistanceSquared(
		const FVector3f& P,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		float& OutAlphaA,
		float& OutAlphaB,
		float& OutAlphaC);
};