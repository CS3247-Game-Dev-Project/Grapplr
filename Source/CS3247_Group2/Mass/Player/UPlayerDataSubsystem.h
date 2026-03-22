#pragma once

#include "CoreMinimal.h"
#include "UPlayerDataSubsystem.generated.h"

UCLASS()
class CS3247_GROUP2_API UPlayerDataSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Stores the location of the player.
	UPROPERTY(BlueprintReadOnly);
	FVector PlayerLocation = FVector::ZeroVector;

	TWeakObjectPtr<AActor> PlayerPtr;
	
	virtual void Tick(float DeltaTime) override
	{
		// Try to find the player if we don't have one yet
		if (!IsValid(PlayerPtr.Get()))
		{
			if (const auto World = GetWorld(); World)
			{
				if (const APlayerController* PC = World->GetFirstPlayerController(); PC && PC->GetPawn())
				{
					PlayerPtr = PC->GetPawn();
				}
			}
		}
		
		// Update the location snapshot
		if (const AActor* CachedPlayer = PlayerPtr.Get(); IsValid(CachedPlayer))
		{
			PlayerLocation = CachedPlayer->GetActorLocation();
		}
	}
	
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerDataSubsystem, STATGROUP_Tickables);
	}
	
	// This ensures the subsystem only ticks in the actual game world, not the editor viewport
	virtual bool IsTickableInEditor() const override { return false; }
};
