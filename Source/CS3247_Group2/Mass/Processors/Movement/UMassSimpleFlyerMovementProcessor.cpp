#include "UMassSimpleFlyerMovementProcessor.h"
#include "MassCommonTypes.h"
#include "MassNavigationFragments.h"
#include "MassEntityManager.h"
#include "MassEntityQuery.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "CS3247_Group2/Mass/Structs/FMovementFragments.h"
#include "CS3247_Group2/Mass/Subsystems/UPlayerDataSubsystem.h"
#include "Kismet/KismetMathLibrary.h"

UMassSimpleFlyerMovementProcessor::UMassSimpleFlyerMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassSimpleFlyerMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);

	EntityQuery.AddTagRequirement<FSimpleFlyerMovementTag>(EMassFragmentPresence::All);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleFlyerMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation](FMassExecutionContext& IterContext)
	{
		const auto MoveTargets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		constexpr float PERSONAL_SPACE_RADIUS = 250.f;
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			auto& MoveTarget = MoveTargets[i];
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector Distance = (PlayerLocation - CurrentLocation);
			
			// Set target to the player.
			MoveTarget.Center = PlayerLocation;
			MoveTarget.Center.Z += Distance.Size2D() / 2.0f; // Simple heuristic for flight.
			if (!Distance.IsNearlyZero()) MoveTarget.Forward = (MoveTarget.Center - CurrentLocation).GetSafeNormal();
			MoveTarget.DistanceToGoal = Distance.Size();
			MoveTarget.IntentAtGoal = EMassMovementAction::Move;
		}
		
		// Manual avoidance implementation (since it is not supported in 3D) 
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			FVector SeparationForce = FVector::ZeroVector;
			FVector CurrentPos = Transforms[i].GetTransform().GetLocation();

			// Simple O(n^2) check (Fine for small chunks, use HashGrid for large swarms)
			// Likely fewer flying enemies since they reach the player more easily and are killed more often.
			for (int32 j = 0; j < IterContext.GetNumEntities(); ++j)
			{
				if (i == j) continue;
				FVector OtherPos = Transforms[j].GetTransform().GetLocation();
				float Dist = FVector::Dist(CurrentPos, OtherPos);
				if (Dist < PERSONAL_SPACE_RADIUS)
				{
					SeparationForce += (CurrentPos - OtherPos).GetSafeNormal() * (PERSONAL_SPACE_RADIUS - Dist);
				}
			}
			auto& MoveTarget = MoveTargets[i];
			MoveTarget.Center += SeparationForce;
		}
	});
}
