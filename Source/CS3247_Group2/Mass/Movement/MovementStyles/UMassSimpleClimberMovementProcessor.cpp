#include "UMassSimpleClimberMovementProcessor.h"
#include "MassCommonTypes.h"
#include "MassNavigationFragments.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "MassExecutionContext.h"
#include "DrawDebugHelpers.h"
#include "MassCommonFragments.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"
#include "CS3247_Group2/Mass/Movement/Avoidance/USpatialGridUpdateProcessor.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

UMassSimpleClimberMovementProcessor::UMassSimpleClimberMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;
	ExecutionOrder.ExecuteAfter.Add(USpatialGridUpdateProcessor::StaticClass()->GetFName());

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassSimpleClimberMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);

	EntityQuery.AddTagRequirement<FSimpleClimberMovementTag>(EMassFragmentPresence::All);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleClimberMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	auto DeltaTime = Context.GetDeltaTimeSeconds();
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation](FMassExecutionContext& IterContext)
	{
		const auto MoveTargets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			auto& MoveTarget = MoveTargets[i];
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector Distance = (PlayerLocation - CurrentLocation);
			
			// Set target to the player.
			MoveTarget.Center = PlayerLocation;
			if (!Distance.IsNearlyZero()) MoveTarget.Forward = (MoveTarget.Center - CurrentLocation).GetSafeNormal();
			MoveTarget.DistanceToGoal = Distance.Size();
			MoveTarget.IntentAtGoal = EMassMovementAction::Move;
			
			// Performs a line trace (raycast) to test if there is a wall in front, if yes, climb up the wall.
			FVector TraceStart = CurrentLocation;
			FVector TraceEnd = CurrentLocation + Distance.GetSafeNormal() * 100.f;
			FCollisionQueryParams Params;
			if (FHitResult Hit; GetWorld()->LineTraceSingleByObjectType(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
			{
				// UE_LOG(LogTemp, Warning, TEXT("trying to move up!!!"));
				DrawDebugLine(
					GetWorld(), 
					TraceStart, 
					TraceEnd, 
					FColor::Green, 
					false,      // Persistent lines (false means it disappears)
					-1.f,       // LifeTime (-1 uses the default frame time)
					0,          // Depth priority
					2.0f        // Thickness
				);
			} 
		}
	});
}
