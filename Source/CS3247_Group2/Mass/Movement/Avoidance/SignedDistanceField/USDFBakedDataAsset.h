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
	float GetDistanceAtWorldPosition(const FVector& WorldPos);
	FVector GetGradientAtWorldPosition(const FVector& WorldPos);
};

/** Converts a 1D Array Index back into a 3D World Position (Voxel Center)
 */
inline FVector USDFBakedDataAsset::IndexToWorld(int32 Index) const
{
	if (!DistanceData.IsValidIndex(Index)) return FVector::ZeroVector;

	// 1. Reverse the flattening math
	int32 Z = Index / (GridDims.X * GridDims.Y);
	int32 Remainder = Index % (GridDims.X * GridDims.Y);
	int32 Y = Remainder / GridDims.X;
	int32 X = Remainder % GridDims.X;

	// 2. Map Grid Coordinates to World Space
	// We add 0.5 to the coordinates to get the CENTER of the voxel
	FVector LocalPos = FVector(X + 0.5f, Y + 0.5f, Z + 0.5f) * VoxelSize;
    
	return WorldBounds.Min + LocalPos;
}

/** Converts a World Position into a 1D Array Index
 */
inline int32 USDFBakedDataAsset::WorldToIndex(const FVector& WorldPos) const
{
	if (!WorldBounds.IsInside(WorldPos)) return INDEX_NONE;

	// 1. Calculate Local Grid Coordinates
	FVector RelativePos = (WorldPos - WorldBounds.Min) / VoxelSize;
    
	int32 X = FMath::FloorToInt(RelativePos.X);
	int32 Y = FMath::FloorToInt(RelativePos.Y);
	int32 Z = FMath::FloorToInt(RelativePos.Z);

	// 2. Bounds Check
	if (X < 0 || X >= GridDims.X || Y < 0 || Y >= GridDims.Y || Z < 0 || Z >= GridDims.Z)
	{
		return INDEX_NONE;
	}

	// 3. Flatten to 1D
	return X + (Y * GridDims.X) + (Z * GridDims.X * GridDims.Y);
}

inline float USDFBakedDataAsset::GetDistanceAtWorldPosition(const FVector& WorldPos)
{
	if (DistanceData.Num() == 0) return 10000.0f;

	// Convert World Position to Local Grid Space
	FVector LocalPos = (WorldPos - WorldBounds.Min) / VoxelSize;
        
	// Get the integer "Base" voxel (the bottom-left-back corner)
	FIntVector Base;
	Base.X = FMath::FloorToInt(LocalPos.X - 0.5f);
	Base.Y = FMath::FloorToInt(LocalPos.Y - 0.5f);
	Base.Z = FMath::FloorToInt(LocalPos.Z - 0.5f);

	// For a quick & dirty version, just return the nearest:
	// (For a production version, you'd use FMath::Lerp between 8 samples)
	auto GetVal = [&](int32 x, int32 y, int32 z) -> float {
		x = FMath::Clamp(x, 0, GridDims.X - 1);
		y = FMath::Clamp(y, 0, GridDims.Y - 1);
		z = FMath::Clamp(z, 0, GridDims.Z - 1);
		return DistanceData[x + (y * GridDims.X) + (z * GridDims.X * GridDims.Y)];
	};

	return GetVal(Base.X, Base.Y, Base.Z);
}

inline FVector USDFBakedDataAsset::GetGradientAtWorldPosition(const FVector& WorldPos)
{
	float V = VoxelSize;
	// Central Difference Method: sample Left/Right, Up/Down, Forward/Back
	float DX = GetDistanceAtWorldPosition(WorldPos + FVector(V, 0, 0)) - GetDistanceAtWorldPosition(WorldPos - FVector(V, 0, 0));
	float DY = GetDistanceAtWorldPosition(WorldPos + FVector(0, V, 0)) - GetDistanceAtWorldPosition(WorldPos - FVector(0, V, 0));
	float DZ = GetDistanceAtWorldPosition(WorldPos + FVector(0, 0, V)) - GetDistanceAtWorldPosition(WorldPos - FVector(0, 0, V));
    
	return FVector(DX, DY, DZ).GetSafeNormal();
}