#pragma once

#include "MassProcessor.h"
#include "UMassObjectGravityProcessor.generated.h"

/**
 * Simulates gravity for the entity object. 
 * This simple implementation just affects the velocity of objects. 
 */
UCLASS()
class CS3247_GROUP2_API UMassObjectGravityProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassObjectGravityProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
