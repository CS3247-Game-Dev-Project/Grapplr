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
	
	FVector2D GetFlowAtLocation(const FVector& Location) const;
	
	void RegisterVolume(AFlowFieldVolume* Volume)
	{
		ActiveVolume = Volume;
	}
	
	FVector2D GetRawVectorFromAsset(int32 X, int32 Y) const;
	float GetRawHeightFromAsset(int32 X, int32 Y) const;
	
	/** Checks if the location is near an edge (with height difference) */
	bool IsLocationNearEdge(const FVector& Location) const;

	float GetGroundHeight() const
	{
		if (!ActiveVolume) return 0;
		return ActiveVolume->GroundHeight;
	}
	
private:
	UPROPERTY()
	AFlowFieldVolume* ActiveVolume;
};
