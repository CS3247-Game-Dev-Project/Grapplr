#pragma once

#include "MassProcessor.h"
#include "UMassSimpleClimberMovementProcessor.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassSimpleClimberMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassSimpleClimberMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
