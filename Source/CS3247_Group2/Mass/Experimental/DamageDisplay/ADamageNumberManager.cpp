#include "ADamageNumberManager.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "UDamageManagerSubsystem.h"

void ADamageNumberManager::BeginPlay()
{
	Super::BeginPlay();
	if (auto* Subsystem = GetWorld()->GetSubsystem<UDamageManagerSubsystem>())
	{
		Subsystem->DamageManager = this;
	}
}

void ADamageNumberManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (auto* Subsystem = World->GetSubsystem<UDamageManagerSubsystem>())
		{
			if (Subsystem->DamageManager == this)
			{
				Subsystem->DamageManager = nullptr;
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ADamageNumberManager::EmitDamageNumbers(const TArray<FVector>& Locations, const TArray<float>& Amounts, const TArray<bool>& IsCritical) const
{
	// Use Niagara Data Interface to push arrays directly to the GPU
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(DamageNiagaraSystem, "User.HitLocations", Locations);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayFloat(DamageNiagaraSystem, "User.DamageAmounts", Amounts);
	UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayBool(DamageNiagaraSystem, "User.IsCritical", IsCritical);
        
	// Tell Niagara to spawn particles based on the number of elements in the array
	DamageNiagaraSystem->SetIntParameter("User.NumHits", Locations.Num());
}