#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "FEnemyWaveStats.generated.h"

USTRUCT(BlueprintType)
struct CS3247_GROUP2_API FEnemyWaveStats
{
	GENERATED_BODY()

	// Configurables per wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 10.f; // DPS (multiplied by delta time)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VisualScale = 1.0f; // The visual scale of the entity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ExperienceDrop = 1;
	
	// Entity Config template asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UMassEntityConfigAsset> EntityConfig;
};
