#include "FMassStateTreeMoveToPlayerTask.h"
#include "MassStateTreeExecutionContext.h"
#include "MassEntityManager.h"
#include "MassEntitySubsystem.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"

// TODO: work with FMassMoveTargetFragment to set target to player location!

EStateTreeRunStatus FMassStateTreeMoveToPlayerTask::EnterState(
	FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	UWorld* World = Context.GetWorld();
	if (!World) return EStateTreeRunStatus::Failed;
	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return EStateTreeRunStatus::Failed;
	FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FMassEntityHandle Entity = MassContext.GetEntity();
	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
	
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMassStateTreeMoveToPlayerTask::Tick(
	FStateTreeExecutionContext& Context,
	const float DeltaTime) const
{
	
	FTransformFragment& Transform = Context.GetExternalData(TransformHandle);
	FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
	
	if (MoveTarget.DistanceToGoal <= TransitionDistance)
	{
		FVector CurrentLocation = Transform.GetTransform().GetLocation();
		MoveTarget.Center = CurrentLocation;
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FMassStateTreeMoveToPlayerTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	UWorld* World = Context.GetWorld();
	if (!World) return;
	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!EntitySubsystem) return;
	FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);
	FMassEntityHandle Entity = MassContext.GetEntity();
	FMassEntityManager& EntityManager = EntitySubsystem->GetMutableEntityManager();
}

bool FMassStateTreeMoveToPlayerTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(MoveTargetHandle);
	return true;
}
