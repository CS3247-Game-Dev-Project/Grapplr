#include "UMassDamageHandler.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "../Structs/FHealthFragments.h"
#include "MassActorSpawnerSubsystem.h"
#include "MassSpawnLocationProcessor.h"
#include "CS3247_Group2/Mass/Structs/FDamageFragments.h"

void UMassDamageHandler::ApplyDamageToEntity(const UObject* WorldContextObject, UMassAgentComponent* AgentComponent,
                                             float HitDamageAmount, bool IsCriticalHit)
{
	// Get the entity handle, safeguard against missing mass agent.
	if (!AgentComponent)
	{
		return;
	}
	FMassEntityHandle EntityHandle = AgentComponent->GetEntityHandle();

	// Get the World
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	// Get the Mass Subsystem
	UMassEntitySubsystem* MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	if (!MassSubsystem)
	{
		return;
	}

	FMassEntityManager& EntityManager = MassSubsystem->GetMutableEntityManager();

	// Update damage fragment
	if (FDamageAccumulatorFragment* DamageFrag = EntityManager.GetFragmentDataPtr<
		FDamageAccumulatorFragment>(EntityHandle))
	{
		DamageFrag->PendingDamage += HitDamageAmount;
		// Update damage display fragment.
		if (FDamageDisplayFragment* DamageDisplayFragment = EntityManager.GetFragmentDataPtr<
			FDamageDisplayFragment>(EntityHandle))
		{
			DamageDisplayFragment->PendingDamage += HitDamageAmount;
			DamageDisplayFragment->bIsCritical |= IsCriticalHit;
			DamageDisplayFragment->bHasPendingDisplay = true;
		} else 	{
			UE_LOG(LogTemp, Warning, TEXT("damage display fragment not found when damage accumulator fragment exists"));
		}
	}
}
