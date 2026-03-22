#include "UMassMovementSpeedProcessor.h"

#include "FMovementFragments.h"
#include "MassExecutionContext.h"
#include "MassCommonTypes.h"
#include "MassMovementFragments.h"
#include "UEnemyMovementSubsystem.h"
#include "CS3247_Group2/Mass/Constants.h"


UMassMovementSpeedProcessor::UMassMovementSpeedProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(TEXT("SpatialGridUpdateProcessor"));
	ExecutionOrder.ExecuteAfter.Add(TEXT("UMassSteerToMoveTargetProcessor"));

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassMovementSpeedProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassDesiredMovementFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMassForceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FMovementSpeedFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassMovementSpeedProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	const UEnemyMovementSubsystem* GlobalManager = GetWorld()->GetSubsystem<UEnemyMovementSubsystem>();
	float GlobalMovementSpeedMult = GlobalManager ? GlobalManager->GlobalMovementSpeedMultiplier : 1.0f;
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, GlobalMovementSpeedMult](FMassExecutionContext& IterContext)
	{
		const auto Velocities  = IterContext.GetMutableFragmentView<FMassVelocityFragment>();
		const auto DesiredMovement  = IterContext.GetMutableFragmentView<FMassDesiredMovementFragment>();
		auto Forces = IterContext.GetMutableFragmentView<FMassForceFragment>();
		const auto MovementSpeeds = IterContext.GetFragmentView<FMovementSpeedFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			float MaxSpeed = MovementSpeeds[i].MaxMovementSpeed * GlobalMovementSpeedMult;
			
			// Set max velocity 
			Velocities[i].Value = Velocities[i].Value.GetClampedToMaxSize(MaxSpeed);
			Forces[i].Value = FVector::ZeroVector;
			
			// Update debug info (these do not affect movement anymore, purely for syncing with max velocity for cleaner debug screen)
			DesiredMovement[i].DesiredVelocity = DesiredMovement[i].DesiredVelocity.GetClampedToMaxSize(MaxSpeed);
		}
	});
}
