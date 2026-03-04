#include "UMassDamageHandler.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "../Structs/FHealthFragments.h"
#include "MassActorSpawnerSubsystem.h"
#include "MassSpawnLocationProcessor.h"

void UMassDamageHandler::ApplyDamageToEntity(const UObject* WorldContextObject, UMassAgentComponent* AgentComponent,
                                             float DamageAmount)
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

	// Check if the entity already has the fragment
	if (FDamageAccumulatorFragment* DamageFrag = EntityManager.GetFragmentDataPtr<
		FDamageAccumulatorFragment>(EntityHandle))
	{
		DamageFrag->PendingDamage += DamageAmount;
	}
	else
	{
		// Add a new instance with the initial damage.
		FDamageAccumulatorFragment DamageAccumulator;
		DamageAccumulator.PendingDamage = DamageAmount;
		EntityManager.Defer().PushCommand<FMassCommandAddFragmentInstances>(EntityHandle, DamageAccumulator);
	}
}
