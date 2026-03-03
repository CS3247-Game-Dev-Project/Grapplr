#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FHealthFragments.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FHealthFragment : public FMassFragment 
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Mass")
	float MaxHealth = 100.0f;
	float CurrentHealth = 100.0f;
};

USTRUCT()
struct CS3247_GROUP2_API FDamageAccumulatorFragment : public FMassFragment 
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	float PendingDamage = 0.0f;
};

USTRUCT()
struct CS3247_GROUP2_API FDeadTag : public FMassTag
{
	GENERATED_BODY()
};
