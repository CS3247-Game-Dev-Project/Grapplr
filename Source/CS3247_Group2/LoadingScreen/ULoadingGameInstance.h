#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Internationalization/Text.h"
#include "ULoadingGameInstance.generated.h"

class UUserWidget;

/**
 * Base game instance that shows a loading widget through MoviePlayer during map travel.
 * Reparent the existing BP_GameInstance to this class and assign LoadingScreenWidgetClass.
 */
UCLASS(Blueprintable)
class CS3247_GROUP2_API ULoadingGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Loading Screen")
	void SkipLoadingScreenOnce();

protected:
	/** Widget shown while OpenLevel / map travel is blocking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loading Screen")
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

	/** Fallback message used when no widget blueprint has been assigned yet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loading Screen")
	FText LoadingMessage = FText::FromString(TEXT("Loading..."));

	/** Keep the widget alive while MoviePlayer is presenting it. */
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ActiveLoadingWidget = nullptr;

	UPROPERTY(Transient)
	bool bSkipNextLoadingScreen = false;

private:
	void BeginLoadingScreen(const FString& MapName);
	void EndLoadingScreen(UWorld* LoadedWorld);
};
