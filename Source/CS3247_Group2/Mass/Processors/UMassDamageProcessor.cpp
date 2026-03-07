#include "UMassDamageProcessor.h"

#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Structs//FHealthFragments.h"

UMassDamageProcessor::UMassDamageProcessor() : EntityQuery(*this)
{
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassDamageProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FDamageAccumulatorFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
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
				// TODO: add more death processing, e.g. death animation, score, drops, etc.
				IterContext.Defer().AddTag<FDeadTag>(IterContext.GetEntity(i));
				IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(
						-1,
						5.0f,
						FColor::Red,
						TEXT("Enemy killed")
					);
				}
			}
		}
	});
}
