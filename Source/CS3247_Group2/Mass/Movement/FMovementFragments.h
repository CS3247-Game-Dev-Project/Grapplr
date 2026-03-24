#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "FMovementFragments.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FMovementSpeedFragment : public FMassFragment
{
	GENERATED_BODY()

	/** The "clamp" to the movement velocity. */
	float MaxMovementSpeed = 200.f;
	
	/** UNUSED: should be more than movement speed for reasonable steering */
	float MaxAcceleration = 250.f;
};

/**
 * Applies sinusoidal "random" drift to the enemy pathfinding.
 */
USTRUCT()
struct CS3247_GROUP2_API FMassDriftFragment : public FMassFragment
{
	GENERATED_BODY()
	
	/** A unique phase offset so entities don't drift in sync, randomized on spawning */
	float PhaseOffset = 0.0f;
	
	/** How strong this specific entity's drift is */
	float DriftIntensity = 1.0f;

	/** The frequency of the sinusoidal drift. */
	float DriftFrequency = 1.0f;
};

/**
 * Stores the height of the entity, used in gravity calculations to prevent clipping through the ground.
 */
USTRUCT()
struct CS3247_GROUP2_API FHeightFragment : public FMassFragment
{
	GENERATED_BODY()

	/** The height of the entity. */
	UPROPERTY(EditAnywhere)
	float Height = 176.f;
};

/**
 * Whether gravity is applied to the entity (e.g. exp, ground enemies).
 * For performance, we can toggle this on/off for exp orbs.
 */
USTRUCT()
struct CS3247_GROUP2_API FGravityTag : public FMassTag
{
	GENERATED_BODY()
};


USTRUCT()
struct CS3247_GROUP2_API FGravityFragment : public FMassFragment
{
	GENERATED_BODY()
	
	/** The downwards velocity accumulated from gravity. */
	float AccumulatedVelocity = 0.0f;
};

/**
 * Custom 3D entity avoidance logic since the default avoidance logic in mass entity is only supported in 2D.
 * 
 * Uses Spatial Hash Grid for O(nk) time complexity, where k is the average number of neighbors in a cell,
 * and n is the total number of entities with this tag.
 */
USTRUCT()
struct CS3247_GROUP2_API FSpatialGridAvoidanceFragment : public FMassFragment
{
	GENERATED_BODY()
	
	/** By how much the entity avoids other entities */
	float AvoidanceSpaceRadius = 150.f;
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
