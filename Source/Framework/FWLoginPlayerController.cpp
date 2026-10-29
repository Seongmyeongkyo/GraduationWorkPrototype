#include "FWLoginPlayerController.h"
#include "FWLoginWidget.h"
#include "FWLobbyWidget.h"
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
	GameplayLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/DefaultMap.DefaultMap")));
}

void AFWLoginPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	MenuWidget = CreateWidget<UFWLoginWidget>(this);
	if (MenuWidget)
	{
		MenuWidget->AddToViewport(100);
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetWidgetToFocus(MenuWidget->GetInitialFocusWidget());
		SetInputMode(InputMode);
	}
}

void AFWLoginPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	GetWorldTimerManager().ClearTimer(MatchCompleteTimer);
	bMatching = false;
	bShowingMatchComplete = false;
	if (LobbyWidget)
	{
		LobbyWidget->RemoveFromParent();
		LobbyWidget = nullptr;
	}
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AFWLoginPlayerController::StartGame()
{
	if (!IsLocalController() || bStartingGame || LobbyWidget)
	{
		return;
	}
	UFWLobbyWidget* NewLobby = CreateWidget<UFWLobbyWidget>(this);
	if (!NewLobby)
	{
		if (MenuWidget) MenuWidget->ShowStatus(LOCTEXT("LobbyUnavailable", "로비를 열지 못했습니다. 다시 시도해 주세요."));
		return;
	}
	LobbyWidget = NewLobby;
	LobbyWidget->SetLocalNickname(LocalPreviewNickname);
	LobbyWidget->AddToViewport(100);
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
		MenuWidget = nullptr;
	}
	RestoreMenuInput();
}

bool AFWLoginPlayerController::TryPrototypeLogin(const FString& Nickname)
{
	if (!bEnablePrototypeLogin || !IsLocalController() || bStartingGame || LobbyWidget) return false;
	const FString DisplayName = Nickname.TrimStartAndEnd().Left(20);
	if (DisplayName.IsEmpty()) return false;
	LocalPreviewNickname = DisplayName;
	StartGame();
	return LobbyWidget != nullptr;
}

void AFWLoginPlayerController::RestoreMenuInput()
{
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	if (LobbyWidget) InputMode.SetWidgetToFocus(LobbyWidget->GetInitialFocusWidget());
	SetInputMode(InputMode);
}

void AFWLoginPlayerController::BeginPrototypeMatch()
{
	if (!IsLocalController() || !LobbyWidget || !LobbyWidget->IsInTeamLobby() || LobbyWidget->IsInvitationOpen() || bStartingGame || bMatching || bShowingMatchComplete) return;
	bMatching = true;
	MatchStartedAt = FPlatformTime::Seconds();
	ActiveMatchDelay = FMath::IsFinite(PrototypeMatchDelaySeconds)
		? FMath::Clamp(PrototypeMatchDelaySeconds, .1f, 60.f) : 5.f;
	LobbyWidget->SetSearching(true, 0.0);
	GetWorldTimerManager().SetTimer(PrototypeMatchTimer, this, &AFWLoginPlayerController::TickPrototypeMatch, .1f, true);
}

void AFWLoginPlayerController::CancelPrototypeMatch()
{
	if (!IsLocalController() || bStartingGame || !bMatching) return;
	bMatching = false;
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	if (LobbyWidget) LobbyWidget->SetSearching(false);
	RestoreMenuInput();
}

void AFWLoginPlayerController::TickPrototypeMatch()
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

void AFWLoginPlayerController::CompletePrototypeMatch()
{
	if (!bMatching || bStartingGame || bShowingMatchComplete || !LobbyWidget) return;
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	bMatching = false;
	bShowingMatchComplete = true;
	LobbyWidget->ShowMatchComplete();
	const float HoldSeconds = FMath::IsFinite(MatchCompleteDisplaySeconds)
		? FMath::Clamp(MatchCompleteDisplaySeconds, .1f, 10.f) : 3.f;
	GetWorldTimerManager().SetTimer(MatchCompleteTimer, this, &AFWLoginPlayerController::ContinueAfterMatchComplete, HoldSeconds, false);
}

void AFWLoginPlayerController::ContinueAfterMatchComplete()
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
	LobbyWidget->SetTravelling();
	SetInputMode(FInputModeGameOnly());
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Destination);
}

void AFWLoginPlayerController::QuitGame()
{
	GetWorldTimerManager().ClearTimer(PrototypeMatchTimer);
	GetWorldTimerManager().ClearTimer(MatchCompleteTimer);
	bMatching = false;
	bShowingMatchComplete = false;
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
