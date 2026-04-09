#include "UMassEnemyDamageProcessor.h"

#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Constants.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/StateTree/UMassEnemyStateTreeProcessor.h"

UMassEnemyDamageProcessor::UMassEnemyDamageProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassEnemyDamageProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FDamageAccumulatorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassEnemyDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(GetWorld());
	
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		const auto HealthList = IterContext.GetMutableFragmentView<FHealthFragment>();
		const auto DamageList = IterContext.GetMutableFragmentView<FDamageAccumulatorFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			HealthList[i].CurrentHealth -= DamageList[i].PendingDamage;
			if (DamageList[i].PendingDamage > 0.0f && GBDebugHitEnemyHealth)
			{
				UE_LOG(LogTemp, Warning, TEXT("health: %f / %f"), HealthList[i].CurrentHealth, HealthList[i].MaxHealth);
			}
			
			DamageList[i].PendingDamage = 0.0f;
			if (HealthList[i].CurrentHealth <= 0.0f)
			{
				Context.Defer().AddTag<FDeadTag>(IterContext.GetEntity(i));
				SignalSubsystem->SignalEntityDeferred(IterContext, MassEnemyStateTree::Signals::EnemyDeath, IterContext.GetEntity(i));
			}
		}
	});
}
