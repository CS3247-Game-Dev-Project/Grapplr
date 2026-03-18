#pragma once
#include "MassEntityQuery.h"
#include "MassProcessor.h"
#include "UMassDamageDisplayProcessor.generated.h"

/**
 * Handles damage numbers visual indicators retrieval.
 */
UCLASS()
class CS3247_GROUP2_API UMassDamageDisplayProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	UMassDamageDisplayProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
};
