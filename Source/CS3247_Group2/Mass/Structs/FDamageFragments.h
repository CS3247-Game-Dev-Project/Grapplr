#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FDamageFragments.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FDamageFragment : public FMassFragment
{
	GENERATED_BODY()

	float Damage = 5.0f;
};
