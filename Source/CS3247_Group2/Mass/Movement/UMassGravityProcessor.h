#pragma once

#include "MassProcessor.h"
#include "UMassGravityProcessor.generated.h"

/**
 * Simulates raw gravity for the entity.
 * Updates the transform directly to avoid conflicting with other processors or missing out on updates.
 */
UCLASS()
class CS3247_GROUP2_API UMassGravityProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassGravityProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
