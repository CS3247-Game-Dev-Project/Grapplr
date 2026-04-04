#pragma once

#include "MassCommonFragments.h"
#include "MassStateTreeTypes.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "CS3247_Group2/Mass/Damage/FHealthFragments.h"
#include "CS3247_Group2/Mass/Player/UPlayerDataSubsystem.h"
#include "FMassEnemyEvaluator.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FMassEnemyInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsAttacking = false;

	UPROPERTY(EditAnywhere, Category = "Output")
	bool bIsDead = false;
};

/** The evaluator only ticks when the state tree that uses it ticks */
USTRUCT()
struct CS3247_GROUP2_API FMassEnemyEvaluator : public FMassStateTreeEvaluatorBase
{
	GENERATED_BODY()

	virtual const UScriptStruct* GetInstanceDataType() const override 
	{ 
		return FMassEnemyInstanceData::StaticStruct(); 
	}
	
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

protected:
	TStateTreeExternalDataHandle<FHealthFragment> HealthHandle;
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FDamageFragment> DamageHandle;
	TStateTreeExternalDataHandle<UPlayerDataSubsystem> PlayerDataSubsystemHandle;
};