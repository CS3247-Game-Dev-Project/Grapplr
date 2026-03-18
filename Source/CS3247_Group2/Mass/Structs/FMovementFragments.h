#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FMovementFragments.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FMovementSpeedFragment : public FMassFragment
{
	GENERATED_BODY()

	float MaxMovementSpeed = 200.f;
	
	/** should be more than movement speed for reasonable steering */
	float MaxAcceleration = 250.f;
};

/**
 * Simple movement directly towards the player.
 * 
 * Low LOD static mesh colliders do not work, resulting in their grouping.
 * Static meshes are also unaffected by gravity.
 */
USTRUCT()
struct CS3247_GROUP2_API FSimpleGroundMovementTag : public FMassTag
{
	GENERATED_BODY()
};

/**
 * WIP
 */
USTRUCT()
struct CS3247_GROUP2_API FSimpleFlyerMovementTag : public FMassTag
{
	GENERATED_BODY()
};

/**
 * WIP
 */
USTRUCT()
struct CS3247_GROUP2_API FSimpleClimberMovementTag : public FMassTag
{
	GENERATED_BODY()
};
