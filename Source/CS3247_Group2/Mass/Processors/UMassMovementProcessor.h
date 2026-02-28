#pragma once

#include "MassProcessor.h"
#include "MassStateTreeSubsystem.h"
#include "UMassMovementProcessor.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()
	
public:
	UMassMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
	
	FMassEntityQuery EntityQuery;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMassStateTreeSubsystem> StSubsystem = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<AActor> Player = nullptr;
};
