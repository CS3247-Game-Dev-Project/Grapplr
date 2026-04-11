#include "ULoadingGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "MoviePlayer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"
namespace
{
	class SLoadingScreenFallback : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLoadingScreenFallback) {}
			SLATE_ARGUMENT(FText, LoadingMessage)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 1.0f))
				[
					SNew(SBox)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(SThrobber)
						]

						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 18.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(InArgs._LoadingMessage)
							.ColorAndOpacity(FLinearColor(0.92f, 0.92f, 0.92f, 1.0f))
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 28))
						]
					]
				]
			];
		}
	};
}

void ULoadingGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &ThisClass::BeginLoadingScreen);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::EndLoadingScreen);
}

void ULoadingGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	ActiveLoadingWidget = nullptr;

	Super::Shutdown();
}

void ULoadingGameInstance::SkipLoadingScreenOnce()
{
	bSkipNextLoadingScreen = true;
}

void ULoadingGameInstance::BeginLoadingScreen(const FString& MapName)
{
	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	if (IsRunningDedicatedServer() || MoviePlayer == nullptr)
	{
		return;
	}

	if (bSkipNextLoadingScreen)
	{
		bSkipNextLoadingScreen = false;
		return;
	}

	FLoadingScreenAttributes LoadingScreen;
	LoadingScreen.bAllowEngineTick = true;
	LoadingScreen.bAutoCompleteWhenLoadingCompletes = true;
	LoadingScreen.MinimumLoadingScreenDisplayTime = 0.25f;

	ActiveLoadingWidget = nullptr;

	if (LoadingScreenWidgetClass)
	{
		ActiveLoadingWidget = CreateWidget<UUserWidget>(this, LoadingScreenWidgetClass);
		if (IsValid(ActiveLoadingWidget))
		{
			LoadingScreen.WidgetLoadingScreen = ActiveLoadingWidget->TakeWidget();
		}
	}

	if (!LoadingScreen.WidgetLoadingScreen.IsValid())
	{
		LoadingScreen.WidgetLoadingScreen =
			SNew(SLoadingScreenFallback)
			.LoadingMessage(LoadingMessage);
	}

	UE_LOG(LogTemp, Log, TEXT("Showing loading screen for map: %s"), *MapName);
	MoviePlayer->SetupLoadingScreen(LoadingScreen);
}

void ULoadingGameInstance::EndLoadingScreen(UWorld* LoadedWorld)
{
	UE_LOG(LogTemp, Log, TEXT("Finished loading map: %s"), LoadedWorld ? *LoadedWorld->GetMapName() : TEXT("Unknown"));
	ActiveLoadingWidget = nullptr;
}
