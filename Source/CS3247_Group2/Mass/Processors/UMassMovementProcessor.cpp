#include "UMassMovementProcessor.h"

#include "Kismet/KismetMathLibrary.h"
#include "MassExecutionContext.h"
#include "GameFramework/Character.h"
#include "MassNavigationFragments.h"
#include "Kismet/GameplayStatics.h"
#include "MassStateTreeFragments.h"

UMassMovementProcessor::UMassMovementProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Movement;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassMovementProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.Initialize(EntityManager);

	// TODO: split by ground/air movement tags.
	EntityQuery.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMassStateTreeInstanceFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());

	// Get the state tree subsystem
	StSubsystem = GetWorld()->GetSubsystem<UMassStateTreeSubsystem>();
	ProcessorRequirements.AddSubsystemRequirement<UMassStateTreeSubsystem>(EMassFragmentAccess::ReadOnly);

	// Get Player
	Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void UMassMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	if (!Player)
	{
		return;
	}
	if (!StSubsystem)
	{
		return;
	}

	// TODO: set tick rate so it only updates the player's location every tick instead of every frame.
	FVector PlayerLocation = Player->GetActorLocation();

	// Iterate through all entities
	EntityQuery.ForEachEntityChunk(Context, [this, PlayerLocation](FMassExecutionContext& IterContext)
	{
		auto MoveTargets = IterContext.GetMutableFragmentView<FMassMoveTargetFragment>();
		auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		auto STInstances = IterContext.GetFragmentView<FMassStateTreeInstanceFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			// Check if the entity is in a specific state
			const FMassStateTreeInstanceFragment& STInstance = STInstances[i];
			if (!STInstance.InstanceHandle.IsValid())
			{
				continue;
			}
			FStateTreeInstanceData* InstanceData = StSubsystem->GetInstanceData(STInstance.InstanceHandle);
			const FStateTreeExecutionState* ExecState = InstanceData->GetExecutionState();
			if (!ExecState)
			{
				continue;
			}
			if (!ExecState || (ExecState->TreeRunStatus != EStateTreeRunStatus::Running))
			{
				continue;
			}

			// TODO: check for a specific state handle? currently just checks if state tree is running.
			// TODO: State tree needs to check if player is close enough before switching states to attack?
			// (Should stop moving if close enough and trigger attack) 
			// if (ExecState->ActiveState == MyChaseStateHandle)

			// NOTE: navmesh is not used in movement currently, possible that enemies get stuck, although mass avoidance helps.
			// TODO: Use navmesh navigation?
			FMassMoveTargetFragment& MoveTarget = MoveTargets[i];
			const FTransform& EntityTransform = Transforms[i].GetTransform();
			FVector CurrentLocation = EntityTransform.GetLocation();

			// FIXME: static meshes (low lod) is unaffected by gravity, so need to fix the issue of them floating to the player/
			// them moving to the same z coordinate as the player
			// Update orientation as well as location
			FVector Direction = (PlayerLocation - CurrentLocation);

			// Set target to the player.
			MoveTarget.Center = PlayerLocation;
			MoveTarget.Center.Z = UKismetMathLibrary::Min(CurrentLocation.Z, PlayerLocation.Z);
			MoveTarget.Forward = Direction / Direction.Size();
			MoveTarget.DistanceToGoal = Direction.Size();

			// Set intent to move so steering processor picks it up
			MoveTarget.IntentAtGoal = EMassMovementAction::Move;
		}
	});
}
