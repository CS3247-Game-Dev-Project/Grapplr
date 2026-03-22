#pragma once

#include "MassProcessor.h"
#include "UGravityProcessor.generated.h"

/**
 * Simulates gravity for the entity.
 */
UCLASS()
class CS3247_GROUP2_API UGravityProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UGravityProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
