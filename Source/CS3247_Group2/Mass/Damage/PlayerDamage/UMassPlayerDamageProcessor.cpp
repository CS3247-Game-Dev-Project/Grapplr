#include "UMassPlayerDamageProcessor.h"
#include "MassCommonFragments.h"
#include "MassCommonTypes.h"
#include "MassExecutionContext.h"
#include "IPlayerDamageable.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"

#include <atomic>

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
}

void UMassPlayerDamageProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UE_LOG(LogTemp, Log, TEXT("PROCESSOR EXECUTING TICK: %s"), *GetName());

	FVector PlayerLocation = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerLocation;
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
			if (DistSq <= FMath::Square(Damages[i].AttackRange)) ChunkDamage += Damages[i].Damage;
		}
		
		TotalDamage += ChunkDamage;
	});
	
	AActor* Player = GetWorld()->GetSubsystem<UPlayerDataSubsystem>()->PlayerPtr.Get();
	if (TotalDamage > 0.0f && Player && IsValid(Player) && Player->GetClass()->ImplementsInterface(UPlayerDamageable::StaticClass()))
	{
		const float DeltaTime = Context.GetDeltaTimeSeconds();
		IPlayerDamageable::Execute_OnPlayerDamaged(Player, TotalDamage * DeltaTime);
	}
}
