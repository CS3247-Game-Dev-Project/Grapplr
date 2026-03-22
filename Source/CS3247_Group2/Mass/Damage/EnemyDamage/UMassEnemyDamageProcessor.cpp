#include "UMassEnemyDamageProcessor.h"

#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"

UMassEnemyDamageProcessor::UMassEnemyDamageProcessor() : EntityQuery(*this)
{
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);

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

	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& IterContext)
	{
		const auto HealthList = IterContext.GetMutableFragmentView<FHealthFragment>();
		const auto DamageList = IterContext.GetMutableFragmentView<FDamageAccumulatorFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			HealthList[i].CurrentHealth -= DamageList[i].PendingDamage;
			DamageList[i].PendingDamage = 0.0f;
			if (HealthList[i].CurrentHealth <= 0.0f)
			{
				// TODO: replace with mass signal subsystem for performance.
				IterContext.Defer().AddTag<FDeadTag>(IterContext.GetEntity(i));
			}
		}
	});
}
