#include "UMassMovementProcessor.h"
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
	
	if (!Player) return;
	if (!StSubsystem) return;

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
			if (!STInstance.InstanceHandle.IsValid()) continue;
			FStateTreeInstanceData* InstanceData = StSubsystem->GetInstanceData(STInstance.InstanceHandle);
			const FStateTreeExecutionState* ExecState = InstanceData->GetExecutionState();
			if (!ExecState) continue;
			if (!ExecState || (ExecState->TreeRunStatus != EStateTreeRunStatus::Running)) continue;
			
			// TODO: check for a specific state handle? currently just checks if state tree is running.
			// if (ExecState->ActiveState == MyChaseStateHandle)
			
			// NOTE: navmesh is not used in movement currently, possible that enemies get stuck, although mass avoidance helps.
			FMassMoveTargetFragment& MoveTarget = MoveTargets[i];
			const FTransform& EntityTransform = Transforms[i].GetTransform();
			
			// Update orientation as well as location
			FVector Direction = (PlayerLocation - EntityTransform.GetLocation());
			float Dist = Direction.Size();
		    
			// Set target to the player.
			MoveTarget.Center = PlayerLocation;
			MoveTarget.Forward = Direction / Dist; // Manual forward vector
			MoveTarget.DistanceToGoal = Dist;
			
			// Set intent to move so steering processor picks it up
			MoveTarget.IntentAtGoal = EMassMovementAction::Move;
		}
	});
}