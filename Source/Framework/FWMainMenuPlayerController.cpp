#include "FWMainMenuPlayerController.h"
#include "FWMainMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "FWMainMenu"

AFWMainMenuPlayerController::AFWMainMenuPlayerController()
{
	bShowMouseCursor = true;
	bAutoManageActiveCameraTarget = false;
	GameplayLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/DefaultMap.DefaultMap")));
}

void AFWMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	MenuWidget = CreateWidget<UFWMainMenuWidget>(this);
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(100);
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetWidgetToFocus(MenuWidget->GetInitialFocusWidget());
		SetInputMode(InputMode);
	}
}

void AFWMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AFWMainMenuPlayerController::StartGame()
{
	if (bStartingGame)
	{
		return;
	}

	const FString PackageName = GameplayLevel.GetLongPackageName();
	if (GameplayLevel.IsNull() || !FPackageName::DoesPackageExist(PackageName))
	{
		UE_LOG(LogTemp, Error, TEXT("[FWMenu] Gameplay map is unavailable: %s"), *PackageName);
		if (MenuWidget)
		{
			MenuWidget->ShowStatus(LOCTEXT("MapUnavailable", "게임 맵을 찾을 수 없습니다. 맵 설정을 확인해 주세요."));
		}
		return;
	}

	bStartingGame = true;
	if (MenuWidget)
	{
		MenuWidget->SetStartingGame();
	}
	SetInputMode(FInputModeGameOnly());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameplayLevel);
}

void AFWMainMenuPlayerController::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
