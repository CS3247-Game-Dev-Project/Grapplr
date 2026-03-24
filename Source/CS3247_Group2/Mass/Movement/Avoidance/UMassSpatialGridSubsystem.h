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
	// Account for dead enemies, since the entity index might not be repeated.
	static constexpr int32 MAX_ENTITIES = MAX_ENEMY_COUNT * 3;
	static constexpr int32 PLAYER_INDEX = MAX_ENTITIES - 1;

	// Linked list design for entity locations.
	// Update processor adds the entries every tick.
	// Fast access on lookup in avoidance processor.
	
	/** Gets the entity index given the hash idx.
	 * The hash idx basically represents a position in the grid. */
	int32 CellHeads[SpatialConfig::TotalCells];
	
	/** Gets the next entity index in the linked list, given the entity index, 
	 * result is -1 if reached the end of chain */
	int32 NextEntity[MAX_ENTITIES];

	/** Gets the location given the entity index */
	FVector EntityLocations[MAX_ENTITIES];

	void Reset()
	{
		FMemory::Memset(CellHeads, -1, sizeof(CellHeads));
	}
	
	FVector GetLocationFromIdx(int32 EntityIdx) const
	{
		return EntityLocations[EntityIdx];
	}

	static __forceinline int32 GetSpatialHashIndex(const FVector& Pos)
	{
		// Convert to integer grid coordinates
		int32 x = FMath::FloorToInt(Pos.X / SpatialConfig::CellSize);
		int32 y = FMath::FloorToInt(Pos.Y / SpatialConfig::CellSize);
		int32 z = FMath::FloorToInt(Pos.Z / SpatialConfig::CellSize);
		
		// Bitwise XOR hash (The "Infinite" Wrap)
		// Using uint32 prevents negative results before the Final Modulo
		const uint32 Hash = (uint32(x) * p1) ^ (uint32(y) * p2) ^ (uint32(z) * p3);

		// Map back to array size
		return int32(Hash % uint32(SpatialConfig::TotalCells));
	}

	/** Used for debugging (displaying grid boxes) */
	static FVector GetCellCenterFromPos(const FVector& Pos)
	{
		// Find the world space "Grid Coordinates"
		float x = FMath::FloorToFloat(Pos.X / SpatialConfig::CellSize);
		float y = FMath::FloorToFloat(Pos.Y / SpatialConfig::CellSize);
		float z = FMath::FloorToFloat(Pos.Z / SpatialConfig::CellSize);

		// Convert back to world space (this gives the "min" corner of the cell)
		FVector MinCorner(
			x * SpatialConfig::CellSize,
			y * SpatialConfig::CellSize,
			z * SpatialConfig::CellSize
		);

		// Add half the CellSize to get the dead center
		return MinCorner + FVector(SpatialConfig::CellSize * 0.5f);
	}
	
	/** Gets the all entities in the radius, centered at the current location */
	void GetEntitiesInRadius(const FVector& CurrentLocation, float Radius, TArray<int32>& OutEntityIndices) const
	{
		float RadiusSq = Radius * Radius;
		float CellSize = SpatialConfig::CellSize;
		
		// Loop 27 cells
		for (int z = -1; z <= 1; ++z) {
			for (int y = -1; y <= 1; ++y) {
				for (int x = -1; x <= 1; ++x) {
						
					int32 HashIdx = GetSpatialHashIndex(CurrentLocation + 
						FVector(x * CellSize, y * CellSize, z * CellSize));

					for (int32 OtherIdx = CellHeads[HashIdx]; OtherIdx != -1; OtherIdx = NextEntity[OtherIdx]) {
						FVector OtherPos = GetLocationFromIdx(OtherIdx);
						FVector ToOther = OtherPos - CurrentLocation;
						float DistSq = ToOther.SizeSquared();
						
						if (DistSq <= RadiusSq) OutEntityIndices.Add(OtherIdx);
					}
				}
			}
		}
	}
	
private:
	// Large Prime Numbers for "Infinite" Hashing
	// Collisions are possible, so we check also the distance between two entities also in the processor
	static const uint32 p1 = 73856093;
	static const uint32 p2 = 19349663;
	static const uint32 p3 = 83492791;
};
