#pragma once
#include "NiagaraComponent.h"

#include "ADamageNumberManager.generated.h"

UCLASS()
class ADamageNumberManager : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Mass|Enemy")
	UNiagaraComponent* DamageNiagaraSystem;

	void EmitDamageNumbers(const TArray<FVector>& Locations, const TArray<float>& Amounts, const TArray<bool>& IsCritical) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
