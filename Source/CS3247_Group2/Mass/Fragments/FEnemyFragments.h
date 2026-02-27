#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FEnemyFragments.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FHealthFragment : public FMassFragment 
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	float MaxHealth = 100.0f;
	float CurrentHealth = 100.0f;
};

USTRUCT()
struct CS3247_GROUP2_API FGroundEnemyTag : public FMassTag 
{
	GENERATED_BODY()
};
