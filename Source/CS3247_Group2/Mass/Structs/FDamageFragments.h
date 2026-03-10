#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FDamageFragments.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FDamageFragment : public FMassFragment
{
	GENERATED_BODY()
	float Damage = 0.0f;
};

USTRUCT()
struct CS3247_GROUP2_API FDamageDisplayFragment : public FMassFragment
{
	GENERATED_BODY()

	float PendingDamage = 0.0f;
	bool bIsCritical = false;
	bool bHasPendingDisplay = false;
};
