#include "UMassDamageDisplayProcessor.h"

#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassDebugger.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Structs/FDamageFragments.h"
#include "CS3247_Group2/Mass/Subsystems/UDamageManagerSubsystem.h"

UMassDamageDisplayProcessor::UMassDamageDisplayProcessor() : EntityQuery(*this)
{
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassDamageDisplayProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FDamageDisplayFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
	
	DMSubsystem = GetWorld()->GetSubsystem<UDamageManagerSubsystem>();
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

		for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		{
			if (DamageDisplayList[i].bHasPendingDisplay)
			{
				Locations.Add(TransformList[i].GetTransform().GetLocation());
				Amounts.Add(DamageDisplayList[i].PendingDamage);
				Crits.Add(DamageDisplayList[i].bIsCritical);

				// Reset for next frame
				DamageDisplayList[i].PendingDamage = 0;
				DamageDisplayList[i].bHasPendingDisplay = false;
			}
		}
	});
	
	// Send all collected data to the Manager in one go
	if (Locations.Num() > 0 && DMSubsystem && DMSubsystem->DamageManager)
	{
		DMSubsystem->DamageManager->EmitDamageNumbers(Locations, Amounts, Crits);
	}
}
