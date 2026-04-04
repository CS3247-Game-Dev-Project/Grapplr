#include "FMassAttackPlayerTask.h"

#include "StateTreeExecutionContext.h"
#include "StateTreeLinker.h"
#include "CS3247_Group2/Mass/Animations/UMassAnimationComponent.h"

bool FMassAttackPlayerTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(DamageHandle);
	Linker.LinkExternalData(ActorHandle);
	return true;
}

EStateTreeRunStatus FMassAttackPlayerTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (!Context.AreContextDataViewsValid()) return EStateTreeRunStatus::Failed;
	
	const FMassActorFragment& ActorFragment = Context.GetExternalData(ActorHandle);
	if (const AActor* Actor = ActorFragment.Get())
	{
		if (UMassAnimationComponent* AnimComp = Actor->FindComponentByClass<UMassAnimationComponent>())
		{
			AnimComp->CurrentAnimState = EEnemyAnimationState::Attack;
		}
	}
	
	FDamageFragment& Damage = Context.GetExternalData(DamageHandle);
	Damage.bIsAttacking = true;
	
	return FMassStateTreeTaskBase::EnterState(Context, Transition);
}

EStateTreeRunStatus FMassAttackPlayerTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	if (!Context.AreContextDataViewsValid()) return EStateTreeRunStatus::Failed;
	
	return FMassStateTreeTaskBase::Tick(Context, DeltaTime);
}

void FMassAttackPlayerTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (!Context.AreContextDataViewsValid()) return;
	
	FDamageFragment& Damage = Context.GetExternalData(DamageHandle);
	Damage.bIsAttacking = false;
	
	return FMassStateTreeTaskBase::ExitState(Context, Transition);
}
