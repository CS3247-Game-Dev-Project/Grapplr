#pragma once

#include "CoreMinimal.h"
#include "MassEntityConfigAsset.h"
#include "FEnemyWaveStats.generated.h"

/** Configurables/constants per wave */
USTRUCT(BlueprintType)
struct CS3247_GROUP2_API FEnemyWaveStats
{
	GENERATED_BODY()
	
	/** Max health of enemy */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health = 100.f;
	
	/** Default speed of the enemy (before speed multiplier) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Speed = 400.f;
	
	/** Velocity interpolation speed of the enemy's desired velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InterpolationSpeed = 3.f;
	
	/** DPS (multiplied by delta time) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 10.f;
	
	/** Attack range of the enemy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange = 150.f;
	
	/** The base height of the mesh. The actual height is after multiplying with VisualScale. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseMeshHeight = 176.f;
	
	/** The visual scale of the entity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float VisualScale = 1.0f;
	
	/** Should scale to the entity's mesh size, used for avoidance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AvoidanceSpaceRadius = 150.f;
	
	/** The amount of experience dropped on death */ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ExperienceDrop = 1;
	
	/** Overwrites the default number of enemies to spawn in that wave. If -1 then no overwrite */ 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int OverwriteSpawnCount = -1;
	
	/** Entity Config template asset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UMassEntityConfigAsset> EntityConfig;
};
