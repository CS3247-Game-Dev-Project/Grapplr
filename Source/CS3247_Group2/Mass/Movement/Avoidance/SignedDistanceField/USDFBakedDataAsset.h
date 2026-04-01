#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USDFBakedDataAsset.generated.h"

UCLASS(BlueprintType)
class CS3247_GROUP2_API USDFBakedDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere, Category="SDF")
	TArray<float> DistanceData;

	UPROPERTY(VisibleAnywhere, Category="SDF")
	FIntVector GridDims;

	UPROPERTY(VisibleAnywhere, Category="SDF")
	FBox WorldBounds;

	UPROPERTY(VisibleAnywhere, Category="SDF")
	float VoxelSize;
	
	FVector IndexToWorld(int32 Index) const;
	int32 WorldToIndex(const FVector& WorldPos) const;
	FVector WorldToGrid(const FVector& WorldPos) const;
	float GetDistanceAtWorldPosition(const FVector& WorldPos) const;
	FVector GetGradientAtWorldPosition(const FVector& WorldPos) const;
};

/** Converts a 1D Array Index back into a 3D World Position (Voxel Center)
 */
inline FVector USDFBakedDataAsset::IndexToWorld(int32 Index) const
{
	if (!DistanceData.IsValidIndex(Index)) return FVector::ZeroVector;

	int32 Z = Index / (GridDims.X * GridDims.Y);
	int32 Remainder = Index % (GridDims.X * GridDims.Y);
	int32 Y = Remainder / GridDims.X;
	int32 X = Remainder % GridDims.X;

	// Center offset (+0.5f) ensures we return the middle of the voxel
	FVector LocalPos = FVector(X + 0.5f, Y + 0.5f, Z + 0.5f) * VoxelSize;
	return WorldBounds.Min + LocalPos;
}

/** Converts a World Position into a 1D Array Index
 */
inline int32 USDFBakedDataAsset::WorldToIndex(const FVector& WorldPos) const
{
	if (!WorldBounds.IsInside(WorldPos)) return INDEX_NONE;

	FVector RelativePos = (WorldPos - WorldBounds.Min) / VoxelSize;
	int32 X = FMath::FloorToInt(RelativePos.X);
	int32 Y = FMath::FloorToInt(RelativePos.Y);
	int32 Z = FMath::FloorToInt(RelativePos.Z);

	if (X < 0 || X >= GridDims.X || Y < 0 || Y >= GridDims.Y || Z < 0 || Z >= GridDims.Z)
	{
		return INDEX_NONE;
	}

	return X + (Y * GridDims.X) + (Z * GridDims.X * GridDims.Y);
}

/** Returns the continuous grid coordinates (e.g., 10.5, 5.2, 0.1) 
 * This allows you to get both the Base Index and the Lerp Alphas in one go.
 */
inline FVector USDFBakedDataAsset::WorldToGrid(const FVector& WorldPos) const
{
	return (WorldPos - WorldBounds.Min) / VoxelSize;
}

inline float USDFBakedDataAsset::GetDistanceAtWorldPosition(const FVector& WorldPos) const
{
	if (DistanceData.Num() == 0) return 10000.0f;

	FVector LocalPos = WorldToGrid(WorldPos);
    
	int32 X0 = FMath::FloorToInt(LocalPos.X);
	int32 Y0 = FMath::FloorToInt(LocalPos.Y);
	int32 Z0 = FMath::FloorToInt(LocalPos.Z);

	float AlphaX = LocalPos.X - X0;
	float AlphaY = LocalPos.Y - Y0;
	float AlphaZ = LocalPos.Z - Z0;

	int32 X1 = FMath::Clamp(X0 + 1, 0, GridDims.X - 1);
	int32 Y1 = FMath::Clamp(Y0 + 1, 0, GridDims.Y - 1);
	int32 Z1 = FMath::Clamp(Z0 + 1, 0, GridDims.Z - 1);
    
	X0 = FMath::Clamp(X0, 0, GridDims.X - 1);
	Y0 = FMath::Clamp(Y0, 0, GridDims.Y - 1);
	Z0 = FMath::Clamp(Z0, 0, GridDims.Z - 1);

	const int32 DimX = GridDims.X;
	const int32 DimXY = GridDims.X * GridDims.Y;

	auto GetVal = [&](int32 x, int32 y, int32 z) {
		return DistanceData[x + (y * DimX) + (z * DimXY)];
	};

	// Trilinear interpolation.
	// Layer 0
	float V000 = GetVal(X0, Y0, Z0);
	float V100 = GetVal(X1, Y0, Z0);
	float V010 = GetVal(X0, Y1, Z0);
	float V110 = GetVal(X1, Y1, Z0);

	// Layer 1
	float V001 = GetVal(X0, Y0, Z1);
	float V101 = GetVal(X1, Y0, Z1);
	float V011 = GetVal(X0, Y1, Z1);
	float V111 = GetVal(X1, Y1, Z1);

	return FMath::Lerp(
		FMath::Lerp(FMath::Lerp(V000, V100, AlphaX), FMath::Lerp(V010, V110, AlphaX), AlphaY),
		FMath::Lerp(FMath::Lerp(V001, V101, AlphaX), FMath::Lerp(V011, V111, AlphaX), AlphaY),
		AlphaZ
	);
}

inline FVector USDFBakedDataAsset::GetGradientAtWorldPosition(const FVector& WorldPos) const
{
	// High-performance Gradient Calculation using Central Difference
	// We use a small epsilon relative to voxel size for accuracy
	const float Epsilon = VoxelSize * 0.1f;

	float DX = GetDistanceAtWorldPosition(WorldPos + FVector(Epsilon, 0, 0)) - GetDistanceAtWorldPosition(WorldPos - FVector(Epsilon, 0, 0));
	float DY = GetDistanceAtWorldPosition(WorldPos + FVector(0, Epsilon, 0)) - GetDistanceAtWorldPosition(WorldPos - FVector(0, Epsilon, 0));
	float DZ = GetDistanceAtWorldPosition(WorldPos + FVector(0, 0, Epsilon)) - GetDistanceAtWorldPosition(WorldPos - FVector(0, 0, Epsilon));

	return FVector(DX, DY, DZ).GetSafeNormal();
}