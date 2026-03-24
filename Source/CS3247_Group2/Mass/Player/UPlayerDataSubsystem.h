#pragma once

#include "CoreMinimal.h"
#include "UPlayerDataSubsystem.generated.h"

// Listen to these broadcast events in blueprints.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEnemyKill, int32, TickKillCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnExpCollect, int32, TickExpAmount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDamage, float, TickDamageAmount);

/** 
 * Handles player related data.
 */
UCLASS()
class CS3247_GROUP2_API UPlayerDataSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Stores the location of the player. */
	UPROPERTY(BlueprintReadOnly);
	FVector PlayerLocation = FVector::ZeroVector;
	
	/** Stores the forward vector of the player. */
	UPROPERTY(BlueprintReadOnly);
	FVector PlayerForward = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Mass|Enemy");
	int32 TotalKills = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mass|Exp");
	int32 TotalExperienceCollected = 0;
	
	UPROPERTY(BlueprintReadOnly, Category = "Mass|Player");
	float TotalDamageTaken = 0;
	
	UPROPERTY(BlueprintAssignable, Category = "Mass|Enemy")
	FOnEnemyKill OnEnemyKill;
	
	UPROPERTY(BlueprintAssignable, Category = "Mass|Exp")
	FOnExpCollect OnExpCollect;

	UPROPERTY(BlueprintAssignable, Category = "Mass|Player")
	FOnPlayerDamage OnPlayerDamage;
	
	/** Avoid using the player ptr in mass processors as it is unsafe. */
	TWeakObjectPtr<AActor> PlayerPtr;
	
	void AddEnemyKills(int32 Amount)
	{
		TotalKills += Amount;
		if (!IsValid(this) || !GetWorld() || GetWorld()->IsBeingCleanedUp()) return;
		if (!OnEnemyKill.IsBound()) return;
		OnEnemyKill.Broadcast(Amount);
	}
	
	void AddExpCollected(int32 Amount)
	{
		TotalExperienceCollected += Amount;
		if (!IsValid(this) || !GetWorld() || GetWorld()->IsBeingCleanedUp()) return;
		if (!OnExpCollect.IsBound()) return;
		OnExpCollect.Broadcast(Amount);
	}
	
	void AddPlayerDamage(float Amount)
	{
		TotalDamageTaken += Amount;
		if (!IsValid(this) || !GetWorld() || GetWorld()->IsBeingCleanedUp()) return;
		if (!OnPlayerDamage.IsBound()) return;
		OnPlayerDamage.Broadcast(Amount);
	}
	
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerDataSubsystem, STATGROUP_Tickables);
	}
	
	virtual void Deinitialize() override
	{
		// Must unbind every listen to be safe, otherwise a null call to delegate will crash the editor.
		// This is only safe because the subsystem only runs in the game world, and not the editor viewport.
		OnPlayerDamage.Clear();
		OnEnemyKill.Clear();
		OnExpCollect.Clear();
	
		Super::Deinitialize();
	}
	
	/** This ensures the subsystem only ticks in the game world, not the editor viewport. */
	virtual bool IsTickableInEditor() const override
	{
		return false;
	}
};
