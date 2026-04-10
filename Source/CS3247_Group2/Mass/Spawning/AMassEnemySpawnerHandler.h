#pragma once

#include "CoreMinimal.h"
#include "FEnemyWaveStats.h"
#include "GameFramework/Actor.h"
#include "MassEntityManager.h"
#include "Components/BoxComponent.h"
#include "AMassEnemySpawnerHandler.generated.h"

// Define a delegate that passes back the spawned handles
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMassSpawnComplete, const TArray<FMassEntityHandle>&, SpawnedEntities);

UCLASS()
class CS3247_GROUP2_API AMassEnemySpawnerHandler : public AActor
{
	GENERATED_BODY()

public:
	AMassEnemySpawnerHandler()
	{
		ValidSpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
		if (ValidSpawnBox)
		{
			ValidSpawnBox->SetBoxExtent(FVector(500.0f, 500.0f, 250.0f));
			ValidSpawnBox->SetCollisionProfileName(TEXT("NoCollision")); 
			ValidSpawnBox->SetCanEverAffectNavigation(false);
		}
	}
	
	/** Trigger the spawning */
	UFUNCTION(BlueprintCallable, Category = "Mass|Enemy")
	void RequestEntitySpawn(FVector SpawnLocation, FEnemyWaveStats EnemyWaveStats, float MinSpawnRadius, float MaxSpawnRadius, int32 NumToSpawn = 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mass|Enemy")
	int ActiveEnemyLimit = 10000;
	
	UPROPERTY(BlueprintAssignable, Category = "Mass|Enemy")
	FOnMassSpawnComplete OnSpawnComplete;
	
	UPROPERTY(VisibleAnywhere, Category = "Mass|Enemy")
	UBoxComponent* ValidSpawnBox;
};
