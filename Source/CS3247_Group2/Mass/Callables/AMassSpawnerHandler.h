#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MassEntityConfigAsset.h"
#include "MassEntityManager.h"
#include "CS3247_Group2/Mass/Structs/FEnemyWaveStats.h"
#include "AMassSpawnerHandler.generated.h"

// Define a delegate that passes back the spawned handles
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMassSpawnComplete, const TArray<FMassEntityHandle>&, SpawnedEntities);

UCLASS()
class CS3247_GROUP2_API AMassSpawnerHandler : public AActor
{
	GENERATED_BODY()

public:
	// Call this to trigger the spawn
	UFUNCTION(BlueprintCallable, Category = "Mass|Enemy")
	void RequestEntitySpawn(FVector SpawnLocation, FEnemyWaveStats EnemyWaveStats, float SpawnRadius, int32 NumToSpawn = 1);

	UPROPERTY(BlueprintAssignable, Category = "Mass|Enemy")
	FOnMassSpawnComplete OnSpawnComplete;

protected:
	UPROPERTY(EditAnywhere, Category = "Mass|Enemy")
	TSoftObjectPtr<UMassEntityConfigAsset> EntityConfig;
};
