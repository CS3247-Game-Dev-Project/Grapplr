#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FMovementFragments.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FMovementSpeedFragment : public FMassFragment
{
	GENERATED_BODY()

	float SpeedMultiplier = 1.0f;
};
