#pragma once
#include "MassActorSubsystem.h"
#include "MassStateTreeTypes.h"
#include "CS3247_Group2/Mass/Damage/FDamageFragments.h"
#include "FMassAttackPlayerTask.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FMassAttackPlayerTaskInstanceData
{
	GENERATED_BODY()
};


USTRUCT()
struct FMassAttackPlayerTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()
protected:
	virtual const UScriptStruct* GetInstanceDataType() const override 
	{ 
		return FMassAttackPlayerTaskInstanceData::StaticStruct(); 
	}
	
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
private:
	TStateTreeExternalDataHandle<FDamageFragment> DamageHandle;
	TStateTreeExternalDataHandle<FMassActorFragment> ActorHandle;
};
