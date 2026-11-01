#include "FWLoginPlayerController.h"
#include "FWLoginWidget.h"
#include "FWMenuSessionSubsystem.h"
#include "UI/FWMenuFade.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FWLogin"

AFWLoginPlayerController::AFWLoginPlayerController()
{
	bShowMouseCursor = true;
	bAutoManageActiveCameraTarget = false;
	LobbyLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/L_Lobby.L_Lobby")));
}

void AFWLoginPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	if (UFWMenuSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UFWMenuSessionSubsystem>())
	{
		Session->ResetLocalDisplayName();
	}
	ShowLoginScreen(FText::GetEmpty());
	BeginScreenFade(false);
}

void AFWLoginPlayerController::ShowLoginScreen(const FText& Message)
{
	if (MenuWidget) MenuWidget->RemoveFromParent();
	MenuWidget = CreateWidget<UFWLoginWidget>(this);
	if (!MenuWidget) return;
	MenuWidget->AddToViewport(100);
	if (!Message.IsEmpty()) MenuWidget->ShowStatus(Message);
	RestoreLoginInput();
}

void AFWLoginPlayerController::RestoreLoginInput()
{
	if (!MenuWidget) return;
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetWidgetToFocus(MenuWidget->GetInitialFocusWidget());
	SetInputMode(InputMode);
}

void AFWLoginPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	GetWorldTimerManager().ClearTimer(LevelTravelTimer);
	if (MenuWidget) { MenuWidget->RemoveFromParent(); MenuWidget = nullptr; }
	Super::EndPlay(EndPlayReason);
}

void AFWLoginPlayerController::StartGame()
{
	if (!IsLocalController() || bStartingGame) return;
	if (LobbyLevel.IsNull() || !FPackageName::DoesPackageExist(LobbyLevel.GetLongPackageName()))
	{
		if (MenuWidget) MenuWidget->ShowStatus(LOCTEXT("LobbyMapMissing", "로비 맵을 찾을 수 없습니다. LobbyLevel 설정을 확인해 주세요."));
		return;
	}
	bStartingGame = true;
	if (MenuWidget) MenuWidget->SetStartingGame();
	BeginScreenFade(true);
}

bool AFWLoginPlayerController::TryPrototypeLogin(const FString& Nickname)
{
	if (!bEnablePrototypeLogin || !IsLocalController() || bStartingGame) return false;
	const FString DisplayName = Nickname.TrimStartAndEnd().Left(20);
	if (DisplayName.IsEmpty()) return false;
	if (UFWMenuSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UFWMenuSessionSubsystem>())
	{
		Session->SetLocalDisplayName(DisplayName);
	}
	StartGame();
	return bStartingGame;
}

void AFWLoginPlayerController::BeginScreenFade(bool bOut)
{
	// 검은 화면 아래의 버튼이 키보드로 눌리지 않도록 페이드 중에는 UI 포커스를 해제합니다.
	SetInputMode(FInputModeGameOnly());
	bFadeOut = bOut;
	FadeStartedAt = FPlatformTime::Seconds();
	ActiveFadeDuration = FWMenuFade::Duration(bOut ? FadeOutSeconds : FadeInSeconds);
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	if (MenuWidget) MenuWidget->SetTransitionOpacity(bOut ? 0.f : 1.f);
	if (ActiveFadeDuration <= 0.f) { TickScreenFade(); return; }
	GetWorldTimerManager().SetTimer(ScreenFadeTimer, this, &AFWLoginPlayerController::TickScreenFade, .016f, true);
}

void AFWLoginPlayerController::TickScreenFade()
{
	if (!MenuWidget) { GetWorldTimerManager().ClearTimer(ScreenFadeTimer); return; }
	const float Progress = ActiveFadeDuration > 0.f
		? float(FMath::Clamp((FPlatformTime::Seconds() - FadeStartedAt) / ActiveFadeDuration, 0.0, 1.0)) : 1.f;
	MenuWidget->SetTransitionOpacity(bFadeOut ? Progress : 1.f - Progress);
	if (Progress < 1.f) return;
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	if (bFadeOut)
	{
		// 검은 화면이 한 프레임 그려진 뒤 로비 레벨을 엽니다.
		GetWorldTimerManager().SetTimer(LevelTravelTimer, this, &AFWLoginPlayerController::OpenLobbyLevel, .016f, false);
	}
	else RestoreLoginInput();
}

void AFWLoginPlayerController::OpenLobbyLevel()
{
	if (!bStartingGame) return;
	if (LobbyLevel.IsNull() || !FPackageName::DoesPackageExist(LobbyLevel.GetLongPackageName()))
	{
		bStartingGame = false;
		ShowLoginScreen(LOCTEXT("LobbyTravelFailed", "로비 맵을 열 수 없습니다. 맵 설정을 확인해 주세요."));
		BeginScreenFade(false);
		return;
	}
	SetInputMode(FInputModeGameOnly());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, LobbyLevel);
}

void AFWLoginPlayerController::QuitGame()
{
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	GetWorldTimerManager().ClearTimer(LevelTravelTimer);
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
