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
};
