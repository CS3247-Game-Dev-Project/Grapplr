#pragma once

#include "MassCommonFragments.h"
#include "MassNavigationFragments.h"
#include "MassStateTreeTypes.h"
#include "StateTreeTaskBase.h"
#include "FMassStateTreeMoveToPlayerTask.generated.h"

USTRUCT()
struct FMassStateTreeMoveToPlayerTaskInstanceData
{
	GENERATED_BODY()

	// Add any runtime-only variables here (e.g., target actor pointers)
	// If you have none, leave it empty but keep the GENERATED_BODY()
	
};

USTRUCT()
struct CS3247_GROUP2_API FMassStateTreeMoveToPlayerTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()
	
protected:
	// Tells StateTree which data struct to use at runtime
	virtual const UScriptStruct* GetInstanceDataType() const override 
	{ 
		return FMassStateTreeMoveToPlayerTaskInstanceData::StaticStruct(); 
	}

	// Called once when state is entered
	virtual EStateTreeRunStatus EnterState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	// Called every tick while state is active
	virtual EStateTreeRunStatus Tick(
		FStateTreeExecutionContext& Context,
		const float DeltaTime) const override;

	// Called when leaving the state
	virtual void ExitState(
		FStateTreeExecutionContext& Context,
		const FStateTreeTransitionResult& Transition) const override;

	// Link external data (Mass fragments/subsystems)
	virtual bool Link(FStateTreeLinker& Linker) override;

public:
	// The minimum distance required between it and the player to transition to some next state.
	UPROPERTY(EditAnywhere, Category = "Task")
	float TransitionDistance = 500.f;

private:
	// Handles to Mass fragments
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;
};
