#pragma once

#include "MassProcessor.h"
#include "USpatialGridUpdateProcessor.generated.h"

/**
 * Builds the spatial grid every tick, 
 * based on the positions of every enemy.
 */
UCLASS()
class CS3247_GROUP2_API USpatialGridUpdateProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	USpatialGridUpdateProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
