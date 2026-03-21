#pragma once

#include "MassProcessor.h"
#include "UMassSimpleFlyerMovementProcessor.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassSimpleFlyerMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassSimpleFlyerMovementProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};

