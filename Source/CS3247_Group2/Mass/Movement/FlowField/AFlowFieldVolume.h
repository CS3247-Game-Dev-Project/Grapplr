#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UFlowFieldDataAsset.h"
#include "Components/BoxComponent.h"
#include "Components/LineBatchComponent.h"
#include "AFlowFieldVolume.generated.h"

/**
 * Handles the baking of height grid and generation of flow vectors.
 * Holds adjustable values.
 */
UCLASS()
class CS3247_GROUP2_API AFlowFieldVolume : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY()
	ULineBatchComponent* MyLineBatcher;
	
	UPROPERTY(EditAnywhere, Category = "FlowField")
	UFlowFieldDataAsset* TargetAsset;
	
	UPROPERTY(VisibleAnywhere, Category = "FlowField")
	UBoxComponent* BoxComponent;

	/** The size of each grid cell. Smaller -> finer grid, but more cells to compute. */
	UPROPERTY(EditAnywhere, Category = "FlowField")
	float CellSize = 100.0f;
	
	UPROPERTY(EditAnywhere, Category = "FlowField")
    TArray<AActor*> ActorsToIgnore;
	
	FVector LastPlayerLocation;
	
	UPROPERTY(EditAnywhere, Category = "FlowField")
	float RecomputeThreshold = 100.0f;
	
	UPROPERTY(EditAnywhere, Category = "FlowField")
	bool bShowDebug = true;
	
	UPROPERTY(EditAnywhere, Category = "FlowField|Debug")
	bool bShowGroundFlowVectors = true;
	
	UPROPERTY(EditAnywhere, Category = "FlowField|Debug")
	float DebugDistance = 2000.0f;
	
	const uint32 UNREACHABLE = 65535;
	const int32 NX[4] = {1, -1, 0, 0};
	const int32 NY[4] = {0, 0, 1, -1};
	
	AFlowFieldVolume();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
    
	UFUNCTION(CallInEditor, Category = "FlowField")
	void BakeFlowField() const;
	
	/** Draws the flow field, if bShowDebug is true */
	void DrawFlowFieldDebug() const;
	
	/** Calling this function updates the flow field(s) if valid. */
	UFUNCTION(BlueprintCallable, Category = "FlowField")
	void UpdateAllFlowFields();
	
	void GenerateTypeSpecificField(const FVector& PlayerLocation, TArray<FVector2D>& OutVectors) const;
};
