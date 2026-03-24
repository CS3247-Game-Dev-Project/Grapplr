#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FEnemyDrops.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FExpDropFragment : public FMassFragment
{
	GENERATED_BODY()
	
	int ExperienceAmount = 1;
};

USTRUCT()
struct CS3247_GROUP2_API FDropStatsFragment : public FMassFragment
{
	GENERATED_BODY()
	
	int ExperienceAmount = 1;
	float LootChance = 0; // Ranges from 0 to 1. 
	int LootTable = -1; // Determines possible drops.
};
