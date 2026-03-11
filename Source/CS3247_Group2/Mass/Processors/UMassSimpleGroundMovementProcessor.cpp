#include "UMassSimpleGroundMovementProcessor.h"
#include "Kismet/KismetMathLibrary.h"
#include "MassExecutionContext.h"
#include "GameFramework/Character.h"
#include "MassNavigationFragments.h"
#include "Kismet/GameplayStatics.h"
#include "CS3247_Group2/Mass/Structs/FMovementFragments.h"

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
	EntityQuery.AddTagRequirement<FSimpleGroundMovementTag>(EMassFragmentPresence::All);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
	
	// Get Player
	Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void UMassSimpleGroundMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	if (!Player)
	{
		return;
	}
	
	FVector PlayerLocation = Player->GetActorLocation();

	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation](FMassExecutionContext& IterContext)
	{
		const auto MoveTargets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			FMassMoveTargetFragment& MoveTarget = MoveTargets[i];
			FVector CurrentLocation = Transforms[i].GetTransform().GetLocation();
			FVector Distance = (PlayerLocation - CurrentLocation);

			// FIXME: static meshes (low lod) is unaffected by gravity, so need to fix the issue of them floating to the player/
			// them moving to the same z coordinate as the player

			// Set target to the player.
			MoveTarget.Center = PlayerLocation;
			MoveTarget.Center.Z = UKismetMathLibrary::Min(CurrentLocation.Z, PlayerLocation.Z);
			// Update orientation as well
			MoveTarget.Forward = Distance.GetSafeNormal();
			MoveTarget.DistanceToGoal = Distance.Size();
			// Set intent to move so steering processor picks it up
			MoveTarget.IntentAtGoal = EMassMovementAction::Move;
		}
	});
}
