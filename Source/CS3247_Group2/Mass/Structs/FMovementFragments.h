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
 * Simple flying movement towards the player.
 * 
 * Each enemy performs a simple line trace forward and moves up if there is a wall in front of it.
 * Otherwise, moves towards the player in a straight line if they have line of sight.
 * 
 * 
 */
USTRUCT()
struct CS3247_GROUP2_API FSimpleFlyerMovementTag : public FMassTag
{
	GENERATED_BODY()
};

/**
 * Moves towards the player.
 * 
 * If there is a wall in front of it, climbs over it, by moving upwards instead, until it has reached the top.
 * 
 */
USTRUCT()
struct CS3247_GROUP2_API FSimpleClimberMovementTag : public FMassTag
{
	GENERATED_BODY()
};
