#pragma once
#include "MassActorSubsystem.h"
#include "MassStateTreeTypes.h"
#include "StateTreeExecutionContext.h"
#include "FMassDeathTask.generated.h"

USTRUCT()
struct CS3247_GROUP2_API FMassDeathTaskInstanceData
{
	GENERATED_BODY()
};

USTRUCT()
struct FMassDeathTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()
	
protected:
	virtual const UScriptStruct* GetInstanceDataType() const override 
	{ 
		return FMassDeathTaskInstanceData::StaticStruct(); 
	}
	
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	TStateTreeExternalDataHandle<FMassActorFragment> ActorHandle;
};
