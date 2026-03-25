#pragma once

#include "MassEntityConfigAsset.h"
#include "MassProcessor.h"
#include "UMassEnemyDeathProcessor.generated.h"

UCLASS()
class CS3247_GROUP2_API UMassEnemyDeathProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UMassEnemyDeathProcessor();

	// Configure via the project settings
	UPROPERTY(EditAnywhere, Category = "Mass|Enemy", Config)
	TSoftObjectPtr<UMassEntityConfigAsset> ExpEntityConfig;
	
protected:
	virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery EntityQuery;
	
private:
	void SpawnExp(TArray<FVector> SpawnLocations, TArray<int> DropExpAmounts, const TSharedPtr<FMassCommandBuffer>& CommandBuffer) const;
};
