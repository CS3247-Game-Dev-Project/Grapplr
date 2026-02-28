#include "UMassHealthProcessor.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Fragments/FEnemyFragments.h"

UMassHealthProcessor::UMassHealthProcessor() : EntityQuery(*this)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassHealthProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadOnly);
	
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassHealthProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& IterContext)
	{
		const TConstArrayView<FHealthFragment> HealthList = IterContext.GetFragmentView<FHealthFragment>();
		
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			if (HealthList[i].CurrentHealth <= 0.0f)
			{
				// FIXME: should use command buffer? deletion doesn't work as intended
				// EntityManager.Defer().DestroyEntity(IterContext.GetEntity(i));
			}
		}
	});
}
