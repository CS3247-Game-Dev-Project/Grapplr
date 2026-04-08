#include "AMassEnemySpawnerHandler.h"
#include "MassEntityConfigAsset.h"
#include "MassCommonFragments.h"
#include "MassSpawnLocationProcessor.h"
#include "MassEntityManager.h"
#include "MassSpawnerSubsystem.h"
#include "MassSpawnerTypes.h"
#include "VisualLogger/VisualLogger.h"
#include "MassActorSubsystem.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "NavigationSystem.h"
#include "UEnemyCountSubsystem.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/EnemyDeath/FEnemyDrops.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/SignedDistanceField/UMassSDFSubsystem.h"

FVector GetValidLocation(const UNavigationSystemV1* NavSys, const FVector& Origin, const float& MinRadius, const float& MaxRadius)
{
	// Get a random direction (2D)
	FVector RandomDir = FMath::VRand();
	RandomDir.Z = 0.0f;
	RandomDir.Normalize();

	// Get a random distance between Min and Max.
	// Using Square Root of a random float ensures uniform distribution in a circle.
	const float RandomDist = FMath::Lerp(MinRadius, MaxRadius, FMath::Sqrt(FMath::FRand()));

	const FVector TargetPoint = Origin + (RandomDir * RandomDist);

	// Project that point onto the NavMesh.
	// NavMesh will look up/down to find a floor
	if (FNavLocation ProjectedLocation; NavSys->ProjectPointToNavigation(TargetPoint, ProjectedLocation, FVector(100.f, 100.f, 100000.f)))
	{
		return ProjectedLocation.Location;
	}

	// Fallback: Search for a random reachable point, with retry (no min radius constraint if fails too many times) 
	if (FNavLocation RandomNavLocation; NavSys->GetRandomPointInNavigableRadius(Origin, MaxRadius, RandomNavLocation))
	{
		UE_LOG(LogTemp, Log, TEXT("Spawning entity via fallback: random reachable point in navmesh (with no min radius if too many retries)"));
		for (int retry = 0; retry < 10; retry++)
		{
			if ((RandomNavLocation.Location - Origin).Size2D() < MinRadius) break;
			NavSys->GetRandomPointInNavigableRadius(Origin, MaxRadius, RandomNavLocation);
		}
		return RandomNavLocation.Location;
	}

	// Fallback: If no nav point found, offset slightly so they aren't stacked
	// FIXME: bug in the enemy spawning outside the wall, enemy is unable to reach the player, vice versa.
	UE_LOG(LogTemp, Log, TEXT("Spawning entity via fallback: fall from random point within radius square bounds "));
	FVector FinalLocation = Origin;
	for (int retry = 0; retry < 10; retry++)
	{
		FinalLocation= Origin + FVector(FMath::RandRange(-MaxRadius, MaxRadius),
											  FMath::RandRange(-MaxRadius, MaxRadius), Origin.Z + 3000.f);
		if ((FinalLocation - Origin).SizeSquared2D() > MinRadius * MinRadius)
		{
			return FinalLocation;
		}
	}
	
	// Just return some location if still failed.
	return FinalLocation;
}

void AMassEnemySpawnerHandler::RequestEntitySpawn(FVector SpawnLocation, FEnemyWaveStats EnemyWaveStats, float MinSpawnRadius, float MaxSpawnRadius, int32 NumToSpawn)
{
	const UWorld* World = GetWorld();
	if (!World) return;

	if (SpawnLocation.ContainsNaN())
	{
		UE_LOG(LogTemp, Error, TEXT("RequestEntitySpawn: called with NaN Location!"));
		return;
	}
	
	if (UMassSDFSubsystem* SDFSubsystem = GetWorld()->GetSubsystem<UMassSDFSubsystem>())
	{
		// Force the subsystem to find the BakeActor right before we spawn
		SDFSubsystem->EnsureHasCalledPostInitialize();
	}
	
	// Overwrite the default spawn count if any.
	if (EnemyWaveStats.OverwriteSpawnCount >= 0)
	{
		NumToSpawn = EnemyWaveStats.OverwriteSpawnCount;
	}
	
	UEnemyCountSubsystem* EnemyCountSubsystem = World->GetSubsystem<UEnemyCountSubsystem>();
	if (!EnemyCountSubsystem || EnemyCountSubsystem->EnemyCount + NumToSpawn >= ActiveEnemyLimit)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestEntitySpawn: Hit enemy count limit of %d, unable to spawn!"), ActiveEnemyLimit);
		return;
	}

	// Load the Config Asset
	const UMassEntityConfigAsset* LoadedConfig = EnemyWaveStats.EntityConfig.LoadSynchronous();
	if (!EnemyWaveStats.EntityConfig.IsValid() || !LoadedConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("RequestEntitySpawn: EnemyWaveStats.EntityConfig should not be null"));
		return;
	}

	// Use navmesh to get valid spawning locations around 
	TArray<FVector> PreCalculatedLocations;
	PreCalculatedLocations.Reserve(NumToSpawn);
	if (const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		for (int32 i = 0; i < NumToSpawn; ++i)
		{
			const float ZOffset = EnemyWaveStats.BaseMeshHeight * EnemyWaveStats.VisualScale * 0.5;
			PreCalculatedLocations.Add(GetValidLocation(NavSys, SpawnLocation, MinSpawnRadius, MaxSpawnRadius) + FVector(0, 0, ZOffset));
		}
	}
	
	// Assume that spawning is successful early to prevent more enemies than our limit to be spawned.
	EnemyCountSubsystem->EnemyCount += NumToSpawn; 

	// Push to the deferred command queue
	TWeakObjectPtr<AMassEnemySpawnerHandler> WeakThis(this);
	const FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[World, WeakThis, PreCalculatedLocations, EnemyWaveStats, NumToSpawn, LoadedConfig](const FMassEntityManager& InEntityManager)
		{
			UMassSpawnerSubsystem* SpawnerSubsystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(World);
			if (!SpawnerSubsystem)
			{
				return;
			}

			const FMassEntityTemplate& EntityTemplate = LoadedConfig->GetOrCreateEntityTemplate(*World);
			
			// Handle Transforms (Standard Mass way)
			FMassTransformsSpawnData TransformData;
			TransformData.Transforms.Reserve(NumToSpawn);
			for (int32 i = 0; i < NumToSpawn; ++i)
			{
				// Explicitly ensure Scale is 1.0f to avoid NIL Matrix Inverse crashes
				FTransform InitialTransform(FQuat::Identity, PreCalculatedLocations[i], FVector::OneVector);
				TransformData.Transforms.Add(InitialTransform);
			}

			// Spawn the Entities
			// Potentially still crashes if spawn location collides with some other, causing NaNs to appear somehow,
			// usually happens on startup only if spawning large number of entities on the same location/on the player.
			TArray<FMassEntityHandle> OutEntities;
			SpawnerSubsystem->SpawnEntities(EntityTemplate.GetTemplateID(), NumToSpawn,
			                                FInstancedStruct::Make(TransformData),
			                                UMassSpawnLocationProcessor::StaticClass(),
			                                OutEntities);

			// Batch initialize custom stats fragments
			for (const FMassEntityHandle& Entity : OutEntities)
			{
				if (InEntityManager.IsEntityActive(Entity))
				{
					if (FHealthFragment* Health = InEntityManager.GetFragmentDataPtr<FHealthFragment>(Entity))
					{
						Health->CurrentHealth = EnemyWaveStats.Health;
						Health->MaxHealth = EnemyWaveStats.Health;
					}
					if (FDamageFragment* Damage = InEntityManager.GetFragmentDataPtr<FDamageFragment>(Entity))
					{
						Damage->Damage = EnemyWaveStats.Damage;
						Damage->AttackRange = EnemyWaveStats.AttackRange;
					}
					if (FDropStatsFragment *DropStats = InEntityManager.GetFragmentDataPtr<FDropStatsFragment>(Entity))
					{
						DropStats->ExperienceAmount = EnemyWaveStats.ExperienceDrop;
					}
					if (FMovementSpeedFragment *MovementSpeedFragment = InEntityManager.GetFragmentDataPtr<FMovementSpeedFragment>(Entity))
					{
						MovementSpeedFragment->MaxMovementSpeed = EnemyWaveStats.Speed;
						MovementSpeedFragment->VelocityInterpolationSpeed = EnemyWaveStats.InterpolationSpeed;
					}
					if (FMassDesiredMovementFragment *MovementFragment = InEntityManager.GetFragmentDataPtr<FMassDesiredMovementFragment>(Entity))
					{
						MovementFragment->DesiredMaxSpeedOverride = EnemyWaveStats.Speed;
					}
					if (FMassMoveTargetFragment *TargetFragment = InEntityManager.GetFragmentDataPtr<FMassMoveTargetFragment>(Entity))
					{
						TargetFragment->DesiredSpeed.Set(EnemyWaveStats.Speed);
						TargetFragment->SlackRadius = 100.f;
					}
					if (FTransformFragment *TransformFragment = InEntityManager.GetFragmentDataPtr<FTransformFragment>(Entity))
					{
						TransformFragment->GetMutableTransform().SetScale3D(FVector::OneVector * EnemyWaveStats.VisualScale);
					}
					if (FSpatialGridAvoidanceFragment *AvoidanceFragment = InEntityManager.GetFragmentDataPtr<FSpatialGridAvoidanceFragment>(Entity))
					{
						AvoidanceFragment->AvoidanceSpaceRadius = EnemyWaveStats.AvoidanceSpaceRadius;
					}
					if (FMassDriftFragment *DriftFragment = InEntityManager.GetFragmentDataPtr<FMassDriftFragment>(Entity))
					{
						DriftFragment->DriftIntensity = FMath::RandRange(0.1f, 0.3f);
						DriftFragment->PhaseOffset = FMath::FRandRange(0.0f, 6.28f);
						DriftFragment->DriftFrequency = FMath::FRandRange(0.1f, 0.5f);
					}
					if (FHeightFragment *HeightFragment = InEntityManager.GetFragmentDataPtr<FHeightFragment>(Entity)) {
						HeightFragment->Height = EnemyWaveStats.BaseMeshHeight * EnemyWaveStats.VisualScale;
					}
					if (FSimpleFlyerConfigFragment *ConfigFragment = InEntityManager.GetFragmentDataPtr<FSimpleFlyerConfigFragment>(Entity))
					{
						ConfigFragment->FlightHeightAmplitude = FMath::RandRange(1000.f, 1500.f);	
						ConfigFragment->FlightHeightFrequency = FMath::RandRange(0.01f, 0.2f);	
					}
				}
			}

			// Notify Blueprints
			AsyncTask(ENamedThreads::GameThread, [WeakThis, OutEntities]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->OnSpawnComplete.Broadcast(OutEntities);
				}
			});
		});
}
