#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FHealthFragment.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FHealthFragment : public FMassFragment 
{
	GENERATED_BODY()
	float CurrentHealth = 100.0f;
	float MaxHealth = 100.0f;
};
