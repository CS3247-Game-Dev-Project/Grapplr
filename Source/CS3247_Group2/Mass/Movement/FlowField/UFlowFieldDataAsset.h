#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UFlowFieldDataAsset.generated.h"

/** Holds the baked flow field data. */
UCLASS()
class CS3247_GROUP2_API UFlowFieldDataAsset : public UDataAsset
{
    GENERATED_BODY()
    
public:
    UPROPERTY(VisibleAnywhere, Category = "FlowField")
    TArray<uint8> BakedCosts;

    UPROPERTY(VisibleAnywhere, Category = "FlowField")
    TArray<float> BakedHeights;

    UPROPERTY(VisibleAnywhere, Category = "FlowField")
    FIntPoint GridDimensions;

    UPROPERTY(VisibleAnywhere, Category = "FlowField")
    FVector GridWorldOrigin;
    
    /** Ground directions pointing toward the goal */
    UPROPERTY(Transient)
    TArray<FVector2D> GroundFlowVectors;
    
    /** Climbing directions pointing toward the goal */
    UPROPERTY(Transient)
    TArray<FVector2D> ClimbFlowVectors;
};
