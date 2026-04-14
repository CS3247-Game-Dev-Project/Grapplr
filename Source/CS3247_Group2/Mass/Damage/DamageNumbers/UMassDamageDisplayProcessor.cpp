#include "UMassDamageDisplayProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassDebugger.h"
#include "MassExecutionContext.h"
#include "UDamageNumberSubsystem.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "CS3247_Group2/Mass/Movement/FMovementFragments.h"

UMassDamageDisplayProcessor::UMassDamageDisplayProcessor() : EntityQuery(*this)
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassDamageDisplayProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FDamageDisplayFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FHeightFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
}

void UMassDamageDisplayProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());
	
	// Collect damage statistics for visual display
	TArray<FVector> Locations;
	TArray<float> Amounts;
	TArray<bool> Crits;
	
	EntityQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext& IterContext)
	{
		const auto DamageDisplayList = IterContext.GetMutableFragmentView<FDamageDisplayFragment>();
		const auto TransformList = IterContext.GetFragmentView<FTransformFragment>();
		const auto HeightList = IterContext.GetFragmentView<FHeightFragment>();

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			
			if (DamageDisplayList[i].bHasPendingDisplay)
			{
				Locations.Add(TransformList[i].GetTransform().GetLocation() + HeightList[i].Height / 4.0f);
				Amounts.Add(DamageDisplayList[i].PendingDamage);
				Crits.Add(DamageDisplayList[i].bIsCritical);

				// Reset for next frame
				DamageDisplayList[i].PendingDamage = 0;
				DamageDisplayList[i].bHasPendingDisplay = false;
			}
		}
	});
	
	// Send all collected data to the Manager in one go
	if (Locations.Num() > 0)
	{
		AsyncTask(ENamedThreads::GameThread, [this, Locations, Amounts, Crits]()
		{
			if (auto* Subsystem = GetWorld()->GetSubsystem<UDamageNumberSubsystem>()) {
				Subsystem->EmitDamageNumbers(Locations, Amounts, Crits);
			}
		});
	}
}
