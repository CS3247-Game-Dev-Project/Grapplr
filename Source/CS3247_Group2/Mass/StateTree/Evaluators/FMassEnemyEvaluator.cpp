#include "FMassEnemyEvaluator.h"
#include "MassStateTreeExecutionContext.h"
#include "StateTreeLinker.h"

bool FMassEnemyEvaluator::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(HealthHandle);
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(DamageHandle);
	Linker.LinkExternalData(PlayerDataSubsystemHandle);
	return true;
}

void FMassEnemyEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FHealthFragment& Health = Context.GetExternalData(HealthHandle);
	const FTransformFragment& Transform = Context.GetExternalData(TransformHandle);
	const FDamageFragment& Damage = Context.GetExternalData(DamageHandle);
	const UPlayerDataSubsystem& PlayerDataSubsystem = Context.GetExternalData(PlayerDataSubsystemHandle);

	FMassEnemyInstanceData& InstanceData = Context.GetInstanceData<FMassEnemyInstanceData>(*this);
	
	InstanceData.bIsDead = Health.CurrentHealth <= 0.0f;
	const FVector ToPlayer = PlayerDataSubsystem.PlayerLocation - Transform.GetTransform().GetLocation();
	InstanceData.bIsAttacking = ToPlayer.SizeSquared() <= FMath::Square(Damage.AttackRange);
}