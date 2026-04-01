#pragma once

#include "USDFBakedDataAsset.h"
#include "UMassSDFSubsystem.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassSDFSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	/** Called by the ASDFBakeActor in the level */
	void RegisterSDFAsset(USDFBakedDataAsset* InAsset)
	{
		if (!InAsset) return;
		TargetAsset = InAsset;
        
		UE_LOG(LogTemp, Log, TEXT("Mass SDF Subsystem: Registered asset %s"), *InAsset->GetName());
	}

	float GetDistanceAtWorldPosition(const FVector& WorldPos) const;
	
	FVector GetGradientAtWorldPosition(const FVector& WorldPos) const;
	
	bool HasTargetAsset() const
	{
		if (!TargetAsset) {
			UE_LOG(LogTemp, Warning, TEXT("Mass SDF Subsystem: No asset registered!"));
			return false;
		}
		return true;
	}
	
private:
	UPROPERTY()
	USDFBakedDataAsset * TargetAsset;
};