#include "FWLobbyPlayerController.h"
#include "FWLobbyWidget.h"
#include "FWMenuSessionSubsystem.h"
#include "UI/FWMenuFade.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "FWLobbyController"

AFWLobbyPlayerController::AFWLobbyPlayerController()
{
	bShowMouseCursor = true;
	bAutoManageActiveCameraTarget = false;
	GameplayLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/DefaultMap.DefaultMap")));
}

void AFWLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;
	LobbyWidget = CreateWidget<UFWLobbyWidget>(this);
	if (!LobbyWidget) return;
	if (UFWMenuSessionSubsystem* Session = GetGameInstance()->GetSubsystem<UFWMenuSessionSubsystem>())
	{
		LobbyWidget->SetLocalNickname(Session->GetLocalDisplayName());
	}
	LobbyWidget->AddToViewport(100);
	RestoreMenuInput();
	BeginScreenFade(false);
}

void AFWLobbyPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	GetWorldTimerManager().ClearTimer(MatchCompleteTimer);
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	GetWorldTimerManager().ClearTimer(LevelTravelTimer);
	bMatching = false;
	bShowingMatchComplete = false;
	if (LobbyWidget) { LobbyWidget->RemoveFromParent(); LobbyWidget = nullptr; }
	Super::EndPlay(EndPlayReason);
}

void AFWLobbyPlayerController::RestoreMenuInput()
{
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (LobbyWidget) InputMode.SetWidgetToFocus(LobbyWidget->GetInitialFocusWidget());
	SetInputMode(InputMode);
}

void AFWLobbyPlayerController::BeginPrototypeMatch()
{
	if (!IsLocalController() || !LobbyWidget || !LobbyWidget->IsInTeamLobby() || LobbyWidget->IsInvitationOpen() || bStartingGame || bMatching || bShowingMatchComplete) return;
	bMatching = true;
	MatchStartedAt = FPlatformTime::Seconds();
	ActiveMatchDelay = FMath::IsFinite(PrototypeMatchDelaySeconds)
		? FMath::Clamp(PrototypeMatchDelaySeconds, .1f, 60.f) : 5.f;
	LobbyWidget->SetSearching(true, 0.0);
	GetWorldTimerManager().SetTimer(PrototypeMatchTimer, this, &AFWLobbyPlayerController::TickPrototypeMatch, .1f, true);
}

void AFWLobbyPlayerController::CancelPrototypeMatch()
{
	if (!IsLocalController() || bStartingGame || !bMatching) return;
	bMatching = false;
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	if (LobbyWidget) LobbyWidget->SetSearching(false);
	RestoreMenuInput();
}

void AFWLobbyPlayerController::TickPrototypeMatch()
{
	if (!bMatching || bStartingGame || !LobbyWidget)
	{
		bMatching = false;
		GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
		return;
	}
	const double Elapsed = FMath::Max(0.0, FPlatformTime::Seconds() - MatchStartedAt);
	LobbyWidget->SetSearching(true, Elapsed);
	if (Elapsed >= ActiveMatchDelay) CompletePrototypeMatch();
}

void AFWLobbyPlayerController::CompletePrototypeMatch()
{
	if (!bMatching || bStartingGame || bShowingMatchComplete || !LobbyWidget) return;
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	bMatching = false;
	bShowingMatchComplete = true;
	LobbyWidget->ShowMatchComplete();
	const float HoldSeconds = FMath::IsFinite(MatchCompleteDisplaySeconds)
		? FMath::Clamp(MatchCompleteDisplaySeconds, .1f, 10.f) : 3.f;
	GetWorldTimerManager().SetTimer(MatchCompleteTimer, this, &AFWLobbyPlayerController::ContinueAfterMatchComplete, HoldSeconds, false);
}

void AFWLobbyPlayerController::ContinueAfterMatchComplete()
{
	if (!bShowingMatchComplete || bStartingGame || !LobbyWidget) return;
	// 캐릭터 선택창이 준비되기 전에는 기존 플레이 맵으로 이동한다.
	const TSoftObjectPtr<UWorld> Destination = CharacterSelectionLevel.IsNull() ? GameplayLevel : CharacterSelectionLevel;

	const FString PackageName = Destination.GetLongPackageName();
	if (Destination.IsNull() || !FPackageName::DoesPackageExist(PackageName))
	{
		UE_LOG(LogTemp, Error, TEXT("[FWLobby] Destination map is unavailable: %s"), *PackageName);
		bShowingMatchComplete = false;
		LobbyWidget->HideMatchComplete();
		RestoreMenuInput();
		LobbyWidget->ShowStatus(LOCTEXT("MapUnavailable", "이동할 맵을 찾을 수 없습니다. 맵 설정을 확인해 주세요."));
		return;
	}

	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	GetWorldTimerManager().ClearTimer(MatchCompleteTimer);
	bMatching = false;
	bStartingGame = true;
	PendingDestination = Destination;
	LobbyWidget->SetTravelling();
	BeginScreenFade(true);
}

void AFWLobbyPlayerController::BeginScreenFade(bool bOut)
{
	SetInputMode(FInputModeGameOnly());
	bFadeOut = bOut;
	FadeStartedAt = FPlatformTime::Seconds();
	ActiveFadeDuration = FWMenuFade::Duration(bOut ? FadeOutSeconds : FadeInSeconds);
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	if (LobbyWidget) LobbyWidget->SetTransitionOpacity(bOut ? 0.f : 1.f);
	if (ActiveFadeDuration <= 0.f) { TickScreenFade(); return; }
	GetWorldTimerManager().SetTimer(ScreenFadeTimer, this, &AFWLobbyPlayerController::TickScreenFade, .016f, true);
}

void AFWLobbyPlayerController::TickScreenFade()
{
	if (!LobbyWidget) { GetWorldTimerManager().ClearTimer(ScreenFadeTimer); return; }
	const float Progress = ActiveFadeDuration > 0.f
		? float(FMath::Clamp((FPlatformTime::Seconds() - FadeStartedAt) / ActiveFadeDuration, 0.0, 1.0)) : 1.f;
	LobbyWidget->SetTransitionOpacity(bFadeOut ? Progress : 1.f - Progress);
	if (Progress < 1.f) return;
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	if (bFadeOut)
	{
		// 검은 UMG가 한 프레임 표시된 다음 레벨을 엽니다.
		GetWorldTimerManager().SetTimer(LevelTravelTimer, this, &AFWLobbyPlayerController::OpenPendingLevel, .016f, false);
	}
	else RestoreMenuInput();
}

void AFWLobbyPlayerController::OpenPendingLevel()
{
	if (!bStartingGame || PendingDestination.IsNull()) return;
	if (!FPackageName::DoesPackageExist(PendingDestination.GetLongPackageName()))
	{
		bStartingGame = false;
		bShowingMatchComplete = false;
		LobbyWidget->HideMatchComplete();
		LobbyWidget->ShowStatus(LOCTEXT("TravelFailed", "이동할 맵을 찾을 수 없습니다."));
		RestoreMenuInput();
		BeginScreenFade(false);
		return;
	}
	SetInputMode(FInputModeGameOnly());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, PendingDestination);
}

void AFWLobbyPlayerController::QuitGame()
{
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	GetWorldTimerManager().ClearTimer(MatchCompleteTimer);
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	GetWorldTimerManager().ClearTimer(LevelTravelTimer);
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE

