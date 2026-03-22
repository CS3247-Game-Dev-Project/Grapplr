#include "UMassExpDropMovementProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "MassNavigationFragments.h"
#include "MassMovementFragments.h"
#include <atomic>

#include "IExpCollectible.h"
#include "CS3247_Group2/Mass/EnemyDeath/FEnemyDrops.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

UMassExpDropMovementProcessor::UMassExpDropMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassExpDropMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FExpDropFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassExpDropMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	std::atomic<int> TotalExperienceGain = 0;
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;

	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation, &TotalExperienceGain](FMassExecutionContext& IterContext)
	{
		auto Velocities = IterContext.GetMutableFragmentView<FMassVelocityFragment>();
		auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		auto ExpDrops = IterContext.GetFragmentView<FExpDropFragment>();
		const bool bHasGravity = IterContext.DoesArchetypeHaveTag<FGravityTag>();
		
		int ExperienceChunk = 0;
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			const FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			float DistanceToPlayer = FVector::Dist(PlayerLocation, CurrentLocation);
			
			// Only move if within the detection radius
			if (DistanceToPlayer > MaxDetectionRadius)
			{
				Velocities[i].Value.X = 0;
				Velocities[i].Value.Y = 0;
				continue;
			}
			
			// Calculate "Closer = Faster" factor (ranges from 0.0 at edge to 1.0 at player)
			float SpeedFactor = 1.0f - (DistanceToPlayer / MaxDetectionRadius);
			Velocities[i].Value = (PlayerLocation - CurrentLocation).GetSafeNormal() * (BaseMaxSpeed * SpeedFactor);
			
			if (DistanceToPlayer < PickupRadius) {
				ExperienceChunk += ExpDrops[i].ExperienceAmount;
				IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));
				continue;
			}
			
			if (!bHasGravity)
			{
				IterContext.Defer().AddTag<FGravityTag>(IterContext.GetEntity(i));
			}
		}
		
		TotalExperienceGain += ExperienceChunk;
	});
	
	AActor* Player = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerPtr.Get();
	if (TotalExperienceGain > 0 && Player && IsValid(Player) && Player->GetClass()->ImplementsInterface(UExpCollectible::StaticClass()))
	{
		IExpCollectible::Execute_OnExperienceCollected(Player, TotalExperienceGain);
	}
}
