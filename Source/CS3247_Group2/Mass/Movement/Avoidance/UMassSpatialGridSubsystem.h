#pragma once
#include "CS3247_Group2/Mass/Constants.h"
#include "UMassSpatialGridSubsystem.generated.h"

static TAutoConsoleVariable<int32> CVarShowMassSpatialGrid(
	TEXT("mass.ShowSpatialGrid"),
	0,
	TEXT("0: Hide Grid, 1: Show Occupied Cells"),
	ECVF_Cheat);

namespace SpatialConfig
{
	constexpr int32 GridRes = 64; // Power of 2 is best (64x64x64 = 262,144 cells)
	constexpr float CellSize = 200.f; // Affects size of each spatial grid cell.
	constexpr int32 TotalCells = GridRes * GridRes * GridRes; // 64^3 = 262,144
}

UCLASS()
class CS3247_GROUP2_API UMassSpatialGridSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 MAX_ENTITIES = MAX_ENEMY_COUNT + 1;
	static constexpr int32 PLAYER_INDEX = MAX_ENTITIES - 1;

	int32 CellHeads[SpatialConfig::TotalCells];
	int32 NextEntity[MAX_ENTITIES];
	FVector EntityLocations[MAX_ENTITIES]; // Cached for lightning fast access

	void Reset()
	{
		FMemory::Memset(CellHeads, -1, sizeof(CellHeads));
	}

	FVector GetLocationFromIdx(int32 Index) const
	{
		return EntityLocations[Index];
	}

	static __forceinline int32 GetGridIndex(const FVector& Pos)
	{
		{
			// Convert to integer grid coordinates
			int32 x = FMath::FloorToInt(Pos.X / SpatialConfig::CellSize);
			int32 y = FMath::FloorToInt(Pos.Y / SpatialConfig::CellSize);
			int32 z = FMath::FloorToInt(Pos.Z / SpatialConfig::CellSize);

			// Large Prime Numbers for "Infinite" Hashing
			// Collisions are possible, so we check also the distance between two entities also in the processor.
			const uint32 p1 = 73856093;
			const uint32 p2 = 19349663;
			const uint32 p3 = 83492791;

			// Bitwise XOR hash (The "Infinite" Wrap)
			// Using uint32 prevents negative results before the Final Modulo
			const uint32 Hash = (uint32(x) * p1) ^ (uint32(y) * p2) ^ (uint32(z) * p3);

			// Map back to array size
			return Hash % SpatialConfig::TotalCells;
		}
	}

	static FVector GetCellCenterFromPos(const FVector& Pos)
	{
		{
			// Find the integer "Grid Coordinates"
			float x = FMath::FloorToFloat(Pos.X / SpatialConfig::CellSize);
			float y = FMath::FloorToFloat(Pos.Y / SpatialConfig::CellSize);
			float z = FMath::FloorToFloat(Pos.Z / SpatialConfig::CellSize);

			// Convert back to World Space (this gives the "min" corner of the cell)
			FVector MinCorner(
				x * SpatialConfig::CellSize,
				y * SpatialConfig::CellSize,
				z * SpatialConfig::CellSize
			);

			// Add half the CellSize to get the dead center
			return MinCorner + FVector(SpatialConfig::CellSize * 0.5f);
		}
	}
};
