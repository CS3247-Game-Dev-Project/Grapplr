#include "UMassMovementSpeedProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "MassMovementFragments.h"
#include "CS3247_Group2/Mass/Structs/FMovementFragments.h"
#include "CS3247_Group2/Mass/Subsystems/UEnemyGlobalSubsystem.h"

UMassMovementSpeedProcessor::UMassMovementSpeedProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Avoidance);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::UpdateWorldFromMass);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassMovementSpeedProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMovementSpeedFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassMovementSpeedProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const UEnemyGlobalSubsystem* GlobalManager = GetWorld()->GetSubsystem<UEnemyGlobalSubsystem>();
	float GlobalMovementSpeedMult = GlobalManager ? GlobalManager->GlobalMovementSpeedMultiplier : 1.0f;
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, GlobalMovementSpeedMult](FMassExecutionContext& IterContext)
	{
		const auto Velocities  = IterContext.GetMutableFragmentView<FMassVelocityFragment>();
		const auto DesiredMovement  = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		const auto MovementSpeeds = IterContext.GetFragmentView<FMovementSpeedFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			// Set max velocity 
			Velocities[i].Value = Velocities[i].Value.GetClampedToMaxSize(MovementSpeeds[i].MaxMovementSpeed * GlobalMovementSpeedMult);
			
			// Update debug info (these do not affect movement anymore, purely for syncing with max velocity for cleaner debug screen)
			DesiredMovement[i].DesiredVelocity = DesiredMovement[i].DesiredVelocity.GetClampedToMaxSize(MovementSpeeds[i].MaxMovementSpeed * GlobalMovementSpeedMult);
		}
	});
}
