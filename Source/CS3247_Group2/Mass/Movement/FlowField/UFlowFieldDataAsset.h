#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UFlowFieldDataAsset.generated.h"

/** 
 * Holds the baked flow field data. 
 * We assume all cells are walkable, and there are no void cells.
 */
UCLASS()
class CS3247_GROUP2_API UFlowFieldDataAsset : public UDataAsset
{
    GENERATED_BODY()
    
public:
    /** The height of each cell, should be baked if the level changes. */
    UPROPERTY(VisibleAnywhere, Category = "FlowField")
    TArray<float> BakedHeights;

    UPROPERTY(VisibleAnywhere, Category = "FlowField")
    FIntPoint GridDimensions;

    UPROPERTY(VisibleAnywhere, Category = "FlowField")
    FVector GridWorldOrigin;
    
    UPROPERTY(VisibleAnywhere, Category = "FlowField")
	float GroundHeight;
    
    /** Ground directions pointing toward the goal */
    UPROPERTY(Transient)
    TArray<FVector2D> GroundFlowVectors;
    
    /** Climbing directions pointing toward the goal */
    UPROPERTY(Transient)
    TArray<FVector2D> ClimbFlowVectors;
};
