#pragma once

#include "CoreMinimal.h"
#include "FEnemyWaveStats.generated.h"

USTRUCT(BlueprintType)
struct CS3247_GROUP2_API FEnemyWaveStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float UniformScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ExperienceDrop = 1.0f;
};
