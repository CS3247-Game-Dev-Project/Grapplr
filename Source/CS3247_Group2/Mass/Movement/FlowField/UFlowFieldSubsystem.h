#pragma once

#include "AFlowFieldVolume.h"
#include "UFlowFieldSubsystem.generated.h"

/** 
 * Flow field subsystem provides access to the baked flow field data volume.
 */
UCLASS()
class CS3247_GROUP2_API UFlowFieldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	
	void WarnIfNoAsset() const
	{
		if (!ActiveVolume)
		{
			UE_LOG(LogTemp, Warning, TEXT("UFlowFieldSubsystem: No AFlowFieldVolume present in the level!"));
		} else if (!ActiveVolume->TargetAsset)
		{
			UE_LOG(LogTemp, Warning, TEXT("UFlowFieldSubsystem: No target asset found! Please create a UFlowDataAsset, and add it into the AFlowFieldVolume!"));
		}
	}
	
	FVector2D GetFlowAtLocation(const FVector& Location, const bool IsGroundFlow = true) const;
	
	float GetCellSize() const
	{
		WarnIfNoAsset();
		if (!ActiveVolume || !ActiveVolume->TargetAsset) return 0.0f;
		return ActiveVolume->CellSize;
	}
	
	void RegisterVolume(AFlowFieldVolume* Volume)
	{
		ActiveVolume = Volume;
	}
	
	FVector2D GetRawVectorFromAsset(int32 X, int32 Y, const bool IsGroundFlow = true) const;
	float GetRawHeightFromAsset(int32 X, int32 Y) const;
	
	/** Checks if the location is near an edge (with height difference) */
	bool IsLocationNearEdge(const FVector& Location) const;

	float GetGroundHeight() const
	{
		WarnIfNoAsset();
		if (!ActiveVolume || !ActiveVolume->TargetAsset) return 0;
		return ActiveVolume->TargetAsset->GroundHeight;
	}
	
	/** Convert World Position to Grid Coordinates */
	FVector2D ConvertToGridCoords(const FVector& Location) const;
	
	float GetHeightAtLocation(const FVector& Location) const
	{
		WarnIfNoAsset();
		if (!ActiveVolume || !ActiveVolume->TargetAsset) return 0.0f;
		auto Coords = ConvertToGridCoords(Location);
		return GetRawHeightFromAsset(Coords.X, Coords.Y);
	}
	
private:
	UPROPERTY()
	AFlowFieldVolume* ActiveVolume;
};
