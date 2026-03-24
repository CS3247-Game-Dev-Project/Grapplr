#pragma once

#include "MassProcessor.h"
#include "USpatialGridAvoidanceProcessor.generated.h"

/**
 * Performs local avoidance (separation) behavior for every enemy,
 * using the generated spatial hash grid.
 */
UCLASS()
class CS3247_GROUP2_API USpatialGridAvoidanceProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USpatialGridAvoidanceProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
