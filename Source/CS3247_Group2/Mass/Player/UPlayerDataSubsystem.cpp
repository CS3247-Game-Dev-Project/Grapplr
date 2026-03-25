#include "UPlayerDataSubsystem.h"

void UPlayerDataSubsystem::Tick(float DeltaTime)
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
		PlayerForward = CachedPlayer->GetActorForwardVector();
	}
}
