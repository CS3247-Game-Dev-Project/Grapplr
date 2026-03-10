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
