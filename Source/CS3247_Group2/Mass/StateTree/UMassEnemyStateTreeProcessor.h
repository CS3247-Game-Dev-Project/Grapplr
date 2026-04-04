#pragma once
#include "MassSignalSubsystem.h"
#include "MassStateTreeProcessors.h"
#include "UObject/NameTypes.h"
#include "MassCommonTypes.h"
#include "UMassEnemyStateTreeProcessor.generated.h"

/**
 * Custom signals that our custom state tree processor subscribes to.
 * 
 * The evaluator is the one deciding which state to transition to, not the signal. 
 * The type of signal isn't useful, except for trigger a tick on tasks in the current state,
 * which is sufficient for state transitions in the state tree asset (set within in the editor).
 */
namespace MassEnemyStateTree::Signals
{
	const FName EnemyAttackInRange = FName(TEXT("Signal.Enemy.AttackInRange"));
	const FName EnemyAttackOutOfRange = FName(TEXT("Signal.Enemy.AttackOutOfRange"));
	const FName EnemyDeath = FName(TEXT("Signal.Enemy.Death"));
}

/**
 * Custom state tree processor in order to subscribe to custom signals. 
 * When a (subscribed) signal is fired, the state tree wakes up and ticks. 
 */
UCLASS()
class CS3247_GROUP2_API UMassEnemyStateTreeProcessor : public UMassStateTreeProcessor
{
	GENERATED_BODY()

public:	
	UMassEnemyStateTreeProcessor()
	{
		bAutoRegisterWithProcessingPhases = true;
		bRequiresGameThreadExecution = true;
		ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Behavior;
	}
	
protected:
	virtual void InitializeInternal(UObject& Owner, const TSharedRef<FMassEntityManager>& EntityManager) override
	{
		Super::InitializeInternal(Owner, EntityManager);
		if (UMassSignalSubsystem* SignalSubsystem = GetWorld()->GetSubsystem<UMassSignalSubsystem>())
		{
			SubscribeToSignal(*SignalSubsystem, MassEnemyStateTree::Signals::EnemyAttackInRange);
			SubscribeToSignal(*SignalSubsystem, MassEnemyStateTree::Signals::EnemyAttackOutOfRange);
			SubscribeToSignal(*SignalSubsystem, MassEnemyStateTree::Signals::EnemyDeath);
		}
	}
	
	virtual void SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FMassSignalNameLookup& EntitySignals) override
	{
		Super::SignalEntities(EntityManager, Context, EntitySignals);
	}
};
