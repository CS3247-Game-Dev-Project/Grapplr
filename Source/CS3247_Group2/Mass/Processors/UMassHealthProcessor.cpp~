#include "UMassHealthProcessor.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Fragments/FEnemyTag.h"
#include "CS3247_Group2/Mass/Fragments/FHealthFragment.h"

UMassHealthProcessor::UMassHealthProcessor()
{
	// Leave this empty or for basic property defaults. 
	bAutoRegisterWithProcessingPhases = true;
}

void UMassHealthProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	// Set up the Query requirements
	EntityQuery.AddRequirement<FHealthFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FEnemyTag>(EMassFragmentPresence::All);

	// Register the query so the processor "owns" it
	EntityQuery.RegisterWithProcessor(*this);
}

void UMassHealthProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(Context, [](FMassExecutionContext& IterContext)
	{
		const TArrayView<FHealthFragment> HealthList = IterContext.GetMutableFragmentView<FHealthFragment>();
        
		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			if (HealthList[i].CurrentHealth <= 0.0f)
			{
				// Deferring the destruction is the correct move!
				IterContext.Defer().DestroyEntity(IterContext.GetEntity(i));
			}
		}
	});
}