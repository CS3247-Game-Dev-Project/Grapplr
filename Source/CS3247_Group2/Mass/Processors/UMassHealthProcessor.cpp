#include "UMassHealthProcessor.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Fragments/FEnemyFragments.h"

UMassHealthProcessor::UMassHealthProcessor() : EntityQuery(*this)
{
	
}

void UMassHealthProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadOnly);
}

void UMassHealthProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& Context)
	{
		const TConstArrayView<FHealthFragment> HealthList = Context.GetFragmentView<FHealthFragment>();
		const int32 NumEntities = Context.GetNumEntities();
		FMassCommandBuffer& CommandBuffer = Context.Defer();
		
		for (int32 i = 0; i < NumEntities; ++i)
		{
			if (HealthList[i].CurrentHealth <= 0.0f)
			{
				CommandBuffer.DestroyEntity(Context.GetEntity(i));
			}
		}
	});
}
