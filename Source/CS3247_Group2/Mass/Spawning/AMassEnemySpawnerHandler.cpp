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
#include "UEnemyCountSubsystem.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/EnemyDeath/FEnemyDrops.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/SignedDistanceField/UMassSDFSubsystem.h"

FVector GetValidLocation(const UBoxComponent* SpawnBox, const FVector& Origin, const float MinRadius, const float MaxRadius)
{
    if (!SpawnBox) return Origin;

    // Get Box Dimensions
    const FVector BoxOrigin = SpawnBox->GetComponentLocation();
    const FVector BoxExtent = SpawnBox->GetScaledBoxExtent();

    const float BoxMinX = BoxOrigin.X - BoxExtent.X;
    const float BoxMaxX = BoxOrigin.X + BoxExtent.X;
    const float BoxMinY = BoxOrigin.Y - BoxExtent.Y;
    const float BoxMaxY = BoxOrigin.Y + BoxExtent.Y;

    // Intersect Box with the Square bounding the MaxRadius.
    // We use FMath::Clamp to ensure SearchMin is never greater than SearchMax,
    // which keeps the point inside the box even if the radius is elsewhere.
    const float SearchMinX = FMath::Clamp(Origin.X - MaxRadius, BoxMinX, BoxMaxX);
    const float SearchMaxX = FMath::Clamp(Origin.X + MaxRadius, BoxMinX, BoxMaxX);
    const float SearchMinY = FMath::Clamp(Origin.Y - MaxRadius, BoxMinY, BoxMaxY);
    const float SearchMaxY = FMath::Clamp(Origin.Y + MaxRadius, BoxMinY, BoxMaxY);

    // Pick a random point in this "best-effort" intersection
    FVector FinalLocation(
        FMath::RandRange(SearchMinX, SearchMaxX),
        FMath::RandRange(SearchMinY, SearchMaxY),
        BoxOrigin.Z + BoxExtent.Z // Directly set to the TOP of the box
    );

    // Enforce Radial Constraints
    FVector Dir2D = FinalLocation - Origin;
    Dir2D.Z = 0.0f;
    float Dist2D = Dir2D.Size();

    // If point is too close (inside Min) or outside circular Max
    if (Dist2D < MinRadius || Dist2D > MaxRadius)
    {
        const float ClampedDist = FMath::Clamp(Dist2D, MinRadius, MaxRadius);
        
        // Handle edge case where Origin is exactly on the random point
        const FVector SafeDir = (Dist2D < 0.001f) ? FVector::ForwardVector : Dir2D / Dist2D;
        
        FinalLocation.X = Origin.X + (SafeDir.X * ClampedDist);
        FinalLocation.Y = Origin.Y + (SafeDir.Y * ClampedDist);

        // 5. FINAL MANDATORY CLAMP: 
        // Ensures the radial push/pull didn't violate the Box boundaries.
        FinalLocation.X = FMath::Clamp(FinalLocation.X, BoxMinX, BoxMaxX);
        FinalLocation.Y = FMath::Clamp(FinalLocation.Y, BoxMinY, BoxMaxY);
    }

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
	for (int32 i = 0; i < NumToSpawn; ++i)
	{
		const float ZOffset = EnemyWaveStats.BaseMeshHeight * EnemyWaveStats.VisualScale * 0.5;
		PreCalculatedLocations.Add(GetValidLocation(ValidSpawnBox, SpawnLocation, MinSpawnRadius, MaxSpawnRadius) + FVector(0, 0, ZOffset));
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
