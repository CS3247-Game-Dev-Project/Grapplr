#include <atomic>
#include "UMassPlayerDamageProcessor.h"
#include "MassCommonFragments.h"
#include "UMassEnemyDamageProcessor.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "CS3247_Group2/Mass/Interfaces/IPlayerDamageable.h"
#include "CS3247_Group2/Mass/Structs/FDamageFragments.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UMassPlayerDamageProcessor::UMassPlayerDamageProcessor() : EntityQuery(*this)
{
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Movement);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONSTRUCTED: %s"), *GetName());
}

void UMassPlayerDamageProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
	EntityQuery.AddRequirement<FDamageFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);

	UE_LOG(LogTemp, Log, TEXT("PROCESSOR CONFIGURED: %s"), *GetName());
	
	// Get Player
	Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void UMassPlayerDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	if (!Player) return;
	
	FVector PlayerLocation = Player->GetActorLocation();
	std::atomic<float> TotalDamage = 0.0f;
	
	EntityQuery.ForEachEntityChunk(Context, [PlayerLocation, &TotalDamage](const FMassExecutionContext& IterContext)
	{
		const auto Damages = IterContext.GetFragmentView<FDamageFragment>();
		const auto Transforms = IterContext.GetFragmentView<FTransformFragment>();
		
		float ChunkDamage = 0.0f;
       
		   for (int32 i = 0; i < IterContext.GetNumEntities(); ++i)
		   {
			  // Optimization: Use DistSquared to avoid expensive Square Root
			  const float DistSq = FVector::DistSquared(PlayerLocation, Transforms[i].GetTransform().GetLocation()); 
			  if (DistSq <= 22500.f) // 150 * 150
			  {  
				 ChunkDamage += Damages[i].Damage;
			  }
		   }
		
		   TotalDamage += ChunkDamage;
	});
	
	// FIXME: Damage calculation is run every tick, instead of every enemy attack.
	if (Player && IsValid(Player) && Player->Implements<UPlayerDamageable>() && TotalDamage > 0.0f)
	{
		// Use the 'Execute_' static wrapper to call the function
		const float DeltaTime = Context.GetDeltaTimeSeconds();
		IPlayerDamageable::Execute_OnPlayerDamaged(Player, TotalDamage * DeltaTime);
	}
}
