#include "FMassMoveToPlayerTask.h"

#include "MassActorSubsystem.h"
#include "StateTreeLinker.h"
#include "StateTreeExecutionContext.h"
#include "CS3247_Group2/Mass/Animations/UMassAnimationComponent.h"

bool FMassMoveToPlayerTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(ActorHandle);
	return true;
}

EStateTreeRunStatus FMassMoveToPlayerTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (!Context.AreContextDataViewsValid()) return EStateTreeRunStatus::Failed;
		
	const FMassActorFragment& ActorFragment = Context.GetExternalData(ActorHandle);
	if (const AActor* Actor = ActorFragment.Get())
	{
		if (UMassAnimationComponent* AnimComp = Actor->FindComponentByClass<UMassAnimationComponent>())
		{
			AnimComp->CurrentAnimState = EEnemyAnimationState::Walk;
		}
	}
	
	return FMassStateTreeTaskBase::EnterState(Context, Transition);
}

EStateTreeRunStatus FMassMoveToPlayerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	if (!Context.AreContextDataViewsValid()) return EStateTreeRunStatus::Failed;
	
	return FMassStateTreeTaskBase::Tick(Context, DeltaTime);
}

void FMassMoveToPlayerTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (!Context.AreContextDataViewsValid()) return;
	
	return FMassStateTreeTaskBase::ExitState(Context, Transition);
}
