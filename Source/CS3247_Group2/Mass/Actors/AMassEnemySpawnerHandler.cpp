#include "AMassEnemySpawnerHandler.h"
#include "MassEntityConfigAsset.h"
#include "MassCommonFragments.h"
#include "MassSpawnLocationProcessor.h"
#include "MassEntityManager.h"
#include "MassSpawnerSubsystem.h"
#include "MassSpawnerTypes.h"
#include "CS3247_Group2/Mass/Structs/FDamageFragments.h"
#include "CS3247_Group2/Mass/Structs/FHealthFragments.h"
#include "VisualLogger/VisualLogger.h"
#include "MassActorSubsystem.h"
#include "NavigationSystem.h"
#include "CS3247_Group2/Mass/Structs/FEnemyDrops.h"

FVector GetValidDonutLocation(const UNavigationSystemV1* NavSys, const FVector& Origin, const float& MinRadius, const float& MaxRadius)
{
	// Get a random direction (2D)
	FVector RandomDir = FMath::VRand();
	RandomDir.Z = 0.0f;
	RandomDir.Normalize();

	// Get a random distance between Min and Max
	// Using Square Root of a random float ensures uniform distribution in a circle
	const float RandomDist = FMath::Lerp(MinRadius, MaxRadius, FMath::Sqrt(FMath::FRand()));

	const FVector TargetPoint = Origin + (RandomDir * RandomDist);

	// Project that point onto the NavMesh
	// The 'Extent' is how far up/down the NavMesh will look to find a floor
	if (FNavLocation ProjectedLocation; NavSys->ProjectPointToNavigation(TargetPoint, ProjectedLocation, FVector(100.f, 100.f, 100000.f)))
	{
		return ProjectedLocation.Location;
	}

	// Fallback: Search for a random reachable point, with no min radius constraint 
	if (FNavLocation RandomNavLocation; NavSys->GetRandomPointInNavigableRadius(Origin, MaxRadius, RandomNavLocation))
	{
		UE_LOG(LogTemp, Warning, TEXT("Spawning entity via fallback: random reachable point in navmesh with no min radius"));
		return RandomNavLocation.Location;
	}

	// Fallback: If no nav point found, offset slightly so they aren't stacked
	UE_LOG(LogTemp, Warning, TEXT("Spawning entity via fallback: random point"));
	return Origin + FVector(FMath::RandRange(-MaxRadius, MaxRadius),
										  FMath::RandRange(-MaxRadius, MaxRadius), MaxRadius);
}

void AMassEnemySpawnerHandler::RequestEntitySpawn(FVector SpawnLocation, FEnemyWaveStats EnemyWaveStats, float MinSpawnRadius, float MaxSpawnRadius, int32 NumToSpawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (SpawnLocation.ContainsNaN())
	{
		UE_LOG(LogTemp, Error, TEXT("RequestEntitySpawn called with NaN Location!"));
		return;
	}

	// Load the Config Asset
	const UMassEntityConfigAsset* LoadedConfig = EntityConfig.LoadSynchronous();
	if (!LoadedConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("RequestEntitySpawn called with null EntityConfig"));
		return;
	}

	// Use navmesh to get valid spawning locations around 
	TArray<FVector> PreCalculatedLocations;
	PreCalculatedLocations.Reserve(NumToSpawn);
	if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		for (int32 i = 0; i < NumToSpawn; ++i)
		{
			PreCalculatedLocations.Add(GetValidDonutLocation(NavSys, SpawnLocation, MinSpawnRadius, MaxSpawnRadius));
			
		}
	}

	// Push to the deferred command queue
	TWeakObjectPtr<AMassEnemySpawnerHandler> WeakThis(this);
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(*World);
	EntityManager.Defer().PushCommand<FMassDeferredSetCommand>(
		[WeakThis, PreCalculatedLocations, EnemyWaveStats, NumToSpawn, LoadedConfig](const FMassEntityManager& InEntityManager)
		{
			UWorld* World = InEntityManager.GetWorld();
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
					}
					if (FDropStatsFragment *DropStats = InEntityManager.GetFragmentDataPtr<FDropStatsFragment>(Entity))
					{
						DropStats->ExperienceAmount = EnemyWaveStats.ExperienceDrop;
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
