#pragma once

#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "USDFBakedDataAsset.h"
#include "ASDFBakeActor.generated.h"

UCLASS()
class CS3247_GROUP2_API ASDFBakeActor : public AActor
{
	GENERATED_BODY()
public:
	ASDFBakeActor();
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category = "SDF")
	USDFBakedDataAsset* TargetAsset;

	UPROPERTY(EditAnywhere, Category = "SDF")
	float VoxelSize = 50.0f;
	
	UFUNCTION(CallInEditor, Category = "SDF")
	void TriggerBake() const;
	
	UPROPERTY(EditAnywhere, Category = "SDF")
	TArray<UClass*> ClassesToIgnore;

	UPROPERTY(EditAnywhere, Category = "SDF|Debug")
	float SliceVisualizationHeight = 100.0f;
	
	UPROPERTY(EditAnywhere, Category = "SDF|Debug")
	float SliceVisualizationLerpDistance = 100.0f;
	
	UPROPERTY(EditAnywhere, Category = "SDF|Debug")
	float VisualizationTime = 10.0f;
	
	UPROPERTY(EditAnywhere, Category = "SDF|Debug")
	float DebugSize = 5.0f;
	
	UPROPERTY(EditAnywhere, Category = "SDF|Debug")
	bool bShowGradients = false;

	// Performance Stride: Increase this (e.g., 2 or 3) for massive levels
	UPROPERTY(EditAnywhere, Category = "SDF|Debug")
	int32 Stride = 1; 
	
	UFUNCTION(CallInEditor, Category = "SDF")
	void VisualizeSDFSlice() const;
	
	UFUNCTION(CallInEditor, Category = "SDF")
	void VisualizeSurface() const;

protected:
	UPROPERTY(VisibleAnywhere)
	UBoxComponent* BakeBoundsVisualizer;
	
	void BakeSignedDistanceField(float InVoxelSize, FBox InBounds) const;

	static void ExtractMeshesFromActor(AActor* RootActor, TArray<UStaticMeshComponent*>& OutMeshComps);
	
	float GetDistanceAtWorldPosition(const FVector& WorldPos) const
	{
		if (!TargetAsset) return 10000.0f;
		return TargetAsset->GetDistanceAtWorldPosition(WorldPos);
	}
	
	FVector GetGradientAtWorldPosition(const FVector& WorldPos) const
	{
		if (!TargetAsset) return FVector::ZeroVector;
		return TargetAsset->GetGradientAtWorldPosition(WorldPos);
	}
};
