#pragma once

#include "MassProcessor.h"
#include "UMassDamageProcessor.generated.h"

/**
 * Handles health/damage logic.
 */
UCLASS()
class CS3247_GROUP2_API UMassDamageProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassDamageProcessor();

protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
