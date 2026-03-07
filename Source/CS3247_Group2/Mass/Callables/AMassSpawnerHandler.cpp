#include "AMassSpawnerHandler.h"
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

void AMassSpawnerHandler::RequestEntitySpawn(FVector SpawnLocation, FEnemyWaveStats EnemyWaveStats, float SpawnRadius, int32 NumToSpawn)
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
			FNavLocation RandomNavLocation;
			// Search for a reachable point
			if (NavSys->GetRandomPointInNavigableRadius(SpawnLocation, SpawnRadius, RandomNavLocation))
			{
				PreCalculatedLocations.Add(RandomNavLocation.Location);
			}
			else
			{
				// Fallback: If no nav point found, offset slightly so they aren't stacked
				UE_LOG(LogTemp, Warning, TEXT("Spawning entity via fallback"));
				FVector Fallback = SpawnLocation + FVector(FMath::RandRange(-SpawnRadius, SpawnRadius),
				                                      FMath::RandRange(-SpawnRadius, SpawnRadius), SpawnRadius);
				PreCalculatedLocations.Add(Fallback);
			}
		}
	}

	// Push to the deferred command queue
	TWeakObjectPtr<AMassSpawnerHandler> WeakThis(this);
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
