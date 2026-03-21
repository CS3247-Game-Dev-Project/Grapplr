#include "UMassSimpleGroundMovementProcessor.h"
#include "Kismet/KismetMathLibrary.h"
#include "MassExecutionContext.h"
#include "MassMovementFragments.h"
#include "MassNavigationFragments.h"
#include "CS3247_Group2/Mass/Structs/FMovementFragments.h"
#include "CS3247_Group2/Mass/Subsystems/UPlayerDataSubsystem.h"
#include "MassStateTreeFragments.h"
#include "MassSignalSubsystem.h"

UMassSimpleGroundMovementProcessor::UMassSimpleGroundMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassSimpleGroundMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	
	// EntityQuery.AddRequirement<FMassStateTreeInstanceFragment>(EMassFragmentAccess::ReadWrite); // TODO
	
	EntityQuery.AddTagRequirement<FSimpleGroundMovementTag>(EMassFragmentPresence::All);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassSimpleGroundMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
	
	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation](FMassExecutionContext& IterContext)
	{
		const auto MoveTargets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		
		// const auto STInstances = IterContext.GetMutableFragmentView<FMassStateTreeInstanceFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			// FIXME: static meshes (low lod) is unaffected by gravity, so need to fix the issue of them floating to the player/
			// them moving to the same z coordinate as the player

			auto& MoveTarget = MoveTargets[i];
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector Distance = (PlayerLocation - CurrentLocation);
			
			// Set target to the player.
			MoveTarget.Center = PlayerLocation;
			MoveTarget.Center.Z = UKismetMathLibrary::Min(CurrentLocation.Z, PlayerLocation.Z);
			if (!Distance.IsNearlyZero()) MoveTarget.Forward = Distance.GetSafeNormal();
			MoveTarget.DistanceToGoal = Distance.Size();
			MoveTarget.IntentAtGoal = EMassMovementAction::Move;
			
			// TODO: Signal nearby enemies to trigger state transition.
			// if (Distance.Size() <= 500.f)
			// {
			// 	UMassSignalSubsystem* SignalSubsystem = GetWorld()->GetSubsystem<UMassSignalSubsystem>();
			// 	if (!SignalSubsystem) return;
			// 	SignalSubsystem->SignalEntity(
			// 	UE::Mass::Signals::StateTreeActivate,
			// 	IterContext.GetEntity(i));
			// }
		}
	});
}
