#pragma once

#include "MassProcessor.h"
#include "UMassExpDropMovementProcessor.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassExpDropMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassExpDropMovementProcessor();
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;

private:
	const float MaxDetectionRadius = 1000.f; 
	const float BaseMaxSpeed = 1200.f;
	const float PickupRadius = 30.f;
};
