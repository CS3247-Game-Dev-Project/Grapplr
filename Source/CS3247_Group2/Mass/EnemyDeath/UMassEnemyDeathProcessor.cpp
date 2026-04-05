#include "UMassEnemyDeathProcessor.h"

#include "FEnemyDrops.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassSpawnerSubsystem.h"
#include "MassSpawnerTypes.h"
#include "MassSpawnLocationProcessor.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"
#include "CS3247_Group2/Mass/Spawning/UEnemyCountSubsystem.h"
#include "CS3247_Group2/Mass/StateTree/UMassEnemyStateTreeProcessor.h"

UMassEnemyDeathProcessor::UMassEnemyDeathProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	bRequiresGameThreadExecution = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
	ExecutionOrder.ExecuteAfter.Add(TEXT("UMassEnemyDamageProcessor"));

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassEnemyDeathProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FDropStatsFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::All);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassEnemyDeathProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	TArray<FVector> ExpSpawnLocations;
	TArray<int> DropExpAmounts;
	int EnemyKillCount = 0;
	TSharedPtr<FMassCommandBuffer> CommandBuffer = Context.GetSharedDeferredCommandBuffer();
	
	EntityQuery.ForEachEntityChunk(Context, [&](const FMassExecutionContext& IterContext)
	{
		const auto TransformList = IterContext.GetFragmentView<FTransformFragment>();
		const auto DropStats = IterContext.GetFragmentView<FDropStatsFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			ExpSpawnLocations.Add( TransformList[i].GetTransform().GetLocation());
			DropExpAmounts.Add(DropStats[i].ExperienceAmount);
			EnemyKillCount++;
			
			// TODO: add death animation?.
			IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));
		}
	});
	
	if (EnemyKillCount > 0)
	{
		AsyncTask(ENamedThreads::GameThread, [this, EnemyKillCount]()
		{
			if (auto* Subsystem = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()) {
				Subsystem->AddEnemyKills(EnemyKillCount);
			}
			if (auto * Subsystem = GetWorld()->GetSubsystem<UEnemyCountSubsystem>())
			{
				Subsystem->EnemyCount -= EnemyKillCount;
			}
		});
	}
	
	SpawnExp(ExpSpawnLocations, DropExpAmounts, CommandBuffer);
}

void UMassEnemyDeathProcessor::SpawnExp(TArray<FVector> SpawnLocations, TArray<int> DropExpAmounts, const TSharedPtr<FMassCommandBuffer>& CommandBuffer) const
{
	if (SpawnLocations.Num() == 0)
	{
		return;
	}
	
	// Load the Config Asset
	if (ExpEntityConfig.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("ExpEntityConfig path is empty!"));
		return;
	}
	const UMassEntityConfigAsset* LoadedConfig = ExpEntityConfig.LoadSynchronous();
	if (!LoadedConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("SpawnExp called with null EntityConfig"));
		return;
	}

	CommandBuffer.Get()->PushCommand<FMassDeferredSetCommand>(
		[SpawnLocations, DropExpAmounts, LoadedConfig](const FMassEntityManager& InEntityManager)
	{
		const UWorld* World = InEntityManager.GetWorld();
		if (!World)
		{
			return;
		}
		UMassSpawnerSubsystem* SpawnerSubsystem = UWorld::GetSubsystem<UMassSpawnerSubsystem>(World);
		if (!SpawnerSubsystem)
		{
			return;
		}

		const FMassEntityTemplate& EntityTemplate = LoadedConfig->GetOrCreateEntityTemplate(*World);
			
		// Base exp amount is treated as 0.
		const float MAX_EXP_AMT = 100.f;
		const float BASE_EXP_SIZE = 0.1f;
		const float MAX_EXP_SIZE = 0.2f;
			
		// Handle Transforms (Standard Mass way)
		FMassTransformsSpawnData TransformData;
		TransformData.Transforms.Reserve(SpawnLocations.Num());
		for (int32 i = 0; i < SpawnLocations.Num(); ++i)
		{
			// Scale down exp orb.
			// Vary sizes/amount depending on experience amount.
			float Alpha = FMath::Min(DropExpAmounts[i], MAX_EXP_AMT) /  MAX_EXP_AMT;
			float SizeMultiplier = FMath::Lerp(BASE_EXP_SIZE, MAX_EXP_SIZE, Alpha);
			FTransform InitialTransform(FQuat::Identity, SpawnLocations[i], FVector::OneVector * SizeMultiplier);
			TransformData.Transforms.Add(InitialTransform);
		}

		// Spawn the Experience Entities
		TArray<FMassEntityHandle> OutEntities;
		SpawnerSubsystem->SpawnEntities(EntityTemplate.GetTemplateID(), SpawnLocations.Num(),
		                                FInstancedStruct::Make(TransformData),
		                                UMassSpawnLocationProcessor::StaticClass(),
		                                OutEntities);

		// Batch initialize custom stats fragments
		for (int32 i = 0; i < OutEntities.Num(); ++i)
		{
			const FMassEntityHandle& Entity = OutEntities[i];
			if (InEntityManager.IsEntityActive(Entity))
			{
				if (FExpDropFragment* ExpDropFragment = InEntityManager.GetFragmentDataPtr<FExpDropFragment>(Entity))
				{
					ExpDropFragment->ExperienceAmount = DropExpAmounts[i];
				}
				if (FTransformFragment *TransformFragment = InEntityManager.GetFragmentDataPtr<FTransformFragment>(Entity))
				{
					if (FHeightFragment* HeightFragment = InEntityManager.GetFragmentDataPtr<FHeightFragment>(Entity))
					{
						HeightFragment->Height = TransformFragment->GetTransform().GetScale3D().Z * 100.f;
					}
				}
			}
		}
	});
}

