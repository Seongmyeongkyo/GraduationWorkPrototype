#include "Frontend/CharacterSelect/FWSelectPlayerController.h"
#include "Frontend/CharacterSelect/FWCharacterSelectWidget.h"
#include "Frontend/FWMenuSessionSubsystem.h"
#include "UI/Common/FWMenuFade.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "FWCharacterSelectController"

AFWSelectPlayerController::AFWSelectPlayerController()
{
	bAutoManageActiveCameraTarget = false;
	GameplayLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/L_Round01_WarmForest_Nature.L_Round01_WarmForest_Nature")));

	// BP 없이 위젯 클래스 지정 (FWPlayerController의 WBP_MiniMap과 같은 방식)
	static ConstructorHelpers::FClassFinder<UFWCharacterSelectWidget> CharacterSelectWidgetFinder(
		TEXT("/Game/UI/CharacterSelect/WBP_CharacterSelect"));
	if (CharacterSelectWidgetFinder.Succeeded())
	{
		CharacterSelectWidgetClass = CharacterSelectWidgetFinder.Class;
	}
}

UFWMenuSessionSubsystem* AFWSelectPlayerController::GetMenuSession() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UFWMenuSessionSubsystem>() : nullptr;
}

bool AFWSelectPlayerController::IsGameplayLevelAvailable() const
{
	return !GameplayLevel.IsNull() && FPackageName::DoesPackageExist(GameplayLevel.GetLongPackageName());
}

void AFWSelectPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	Phase = EFWCharacterSelectPhase::Initializing;
	UFWMenuSessionSubsystem* Session = GetMenuSession();

	if (!CharacterSelectWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[FWCharacterSelect] CharacterSelectWidgetClass is not set on %s."), *GetClass()->GetName());
		Phase = EFWCharacterSelectPhase::Failed;
		return;
	}

	WidgetRef = CreateWidget<UFWCharacterSelectWidget>(this, CharacterSelectWidgetClass);
	if (!WidgetRef)
	{
		Phase = EFWCharacterSelectPhase::Failed;
		return;
	}

	WidgetRef->OnConfirmRequested.AddDynamic(this, &AFWSelectPlayerController::RequestConfirmSelection);
	WidgetRef->AddToViewport(100);
	RestoreSelectInput();
	WidgetRef->InitializeScreen(Session ? Session->GetLocalDisplayName() : FString());
	BeginScreenFade(false);

	// 위젯을 다시 만든 것만으로 선택 세션을 초기화하지 않는다.
	// 새 선택 세션의 초기화는 로비의 매칭 완료 시점에 하고, 여기서 확정값이 남아 있으면 복구 상황으로 본다.
	if (Session && Session->IsCharacterSelectionConfirmed())
	{
		const FName SavedCharacterId = Session->GetSelectedCharacterId();
		if (WidgetRef->ApplyCharacterDisplay(SavedCharacterId))
		{
			Phase = EFWCharacterSelectPhase::Confirmed;
			WidgetRef->SetConfirmedState();
			if (bUseLocalPreviewFlow)
			{
				ScheduleLocalGameplayStart();
			}
			return;
		}
		// 로컬 데이터에 없는 저장값은 쓰지 않음
		Session->ResetCharacterSelection();
	}

	// 기본 캐릭터 행 조회/선택 가능 여부 확인 후 미리보기
	FName InitialCharacterId = DefaultCharacterId;
	if (!WidgetRef->IsSelectableCharacter(InitialCharacterId))
	{
		InitialCharacterId = WidgetRef->FindFirstSelectableCharacterId();
	}
	if (InitialCharacterId.IsNone() || !WidgetRef->PreviewCharacter(InitialCharacterId))
	{
		Phase = EFWCharacterSelectPhase::Failed;
		WidgetRef->SetFailedState(LOCTEXT("NoSelectableCharacter", "선택 가능한 캐릭터가 없습니다. 데이터 테이블을 확인해 주세요."));
		return;
	}

	Phase = EFWCharacterSelectPhase::Selecting;

	// 단독 검증 모드일 때만 로컬 제한 시간 시작 (타이머는 위젯 Construct가 아니라 여기서 한 번만 시작)
	if (bUseLocalPreviewFlow)
	{
		StartLocalSelectionTimer();
	}
}

void AFWSelectPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FTimerManager& TimerManager = GetWorldTimerManager();
	TimerManager.ClearTimer(SelectionTimerHandle);
	TimerManager.ClearTimer(LocalStartTimerHandle);
	TimerManager.ClearTimer(ScreenFadeTimer);
	TimerManager.ClearTimer(LevelTravelTimer);

	if (WidgetRef)
	{
		WidgetRef->OnConfirmRequested.RemoveDynamic(this, &AFWSelectPlayerController::RequestConfirmSelection);
		WidgetRef->RemoveFromParent();
		WidgetRef = nullptr;
	}

	// ResetCharacterSelection()은 호출하지 않음: Nature가 읽을 확정 정보를 유지
	Super::EndPlay(EndPlayReason);
}

void AFWSelectPlayerController::RestoreSelectInput()
{
	bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

// ---------------------------------------------------------------- 확정 요청 / 결과

void AFWSelectPlayerController::RequestConfirmSelection(FName CharacterId)
{
	// 위젯은 로컬 컨트롤러에서만 만들어지므로 WidgetRef 검사로 충분
	if (!WidgetRef)
	{
		return;
	}

	// 1. 버튼 연타, 대기 중 재요청 차단
	if (Phase != EFWCharacterSelectPhase::Selecting)
	{
		return;
	}

	// 2. 캐릭터 행이 있고 선택 가능한지
	if (!WidgetRef->IsSelectableCharacter(CharacterId))
	{
		WidgetRef->ShowSelectionError(LOCTEXT("InvalidCharacter", "선택할 수 없는 캐릭터입니다."));
		return;
	}

	// 3. 게임 진입에 필요한 설정 확인
	if (!IsGameplayLevelAvailable())
	{
		UE_LOG(LogTemp, Error, TEXT("[FWCharacterSelect] GameplayLevel is unavailable: %s"), *GameplayLevel.GetLongPackageName());
		WidgetRef->ShowSelectionError(LOCTEXT("GameplayLevelMissing", "이동할 게임 맵을 찾을 수 없습니다. GameplayLevel 설정을 확인해 주세요."));
		return;
	}

	// 4~7. 대기 상태로 전환하고 입력 잠금
	Phase = EFWCharacterSelectPhase::ConfirmPending;
	WidgetRef->SetRequestPending(true);

	if (bUseLocalPreviewFlow)
	{
		ApplyConfirmedSelection(CharacterId);
		if (Phase == EFWCharacterSelectPhase::Confirmed)
		{
			ScheduleLocalGameplayStart();
		}
		return;
	}

	// 실제 연동 모드: 요청만 넘기고 결과(ApplyConfirmedSelection / ApplyConfirmationRejected)를 기다림
	OnSelectionConfirmRequested.Broadcast(CharacterId);
}

void AFWSelectPlayerController::ApplyConfirmedSelection(FName CharacterId)
{
	if (!WidgetRef)
	{
		return;
	}

	// 1. 이동 중이거나 진행 불가 상태면 무시
	if (Phase == EFWCharacterSelectPhase::Travelling || Phase == EFWCharacterSelectPhase::Failed)
	{
		return;
	}

	UFWMenuSessionSubsystem* Session = GetMenuSession();

	// 같은 확정 결과가 다시 오면 아무것도 하지 않음
	if (Phase == EFWCharacterSelectPhase::Confirmed && Session && Session->IsCharacterSelectionConfirmed()
		&& Session->GetSelectedCharacterId() == CharacterId)
	{
		return;
	}

	// 2. 최종 ID에 대응하는 로컬 데이터가 없으면 오류 표시 후 게임 진입 중단
	if (!Session || !WidgetRef->FindCharacterData(CharacterId))
	{
		UE_LOG(LogTemp, Error, TEXT("[FWCharacterSelect] Confirmed character '%s' has no local data."), *CharacterId.ToString());
		Phase = EFWCharacterSelectPhase::Failed;
		StopLocalSelectionTimer();
		WidgetRef->SetFailedState(LOCTEXT("ConfirmedDataMissing", "확정된 캐릭터 정보를 찾을 수 없어 게임에 진입할 수 없습니다."));
		return;
	}

	// 3. 서브시스템에 저장
	Session->ConfirmCharacterSelection(CharacterId);

	// 4~5. 상태 전환, 단독 검증용 타이머 정리
	Phase = EFWCharacterSelectPhase::Confirmed;
	StopLocalSelectionTimer();

	// 6~8. 최종 ID 표시(미리보기와 달라도) → 대기 해제 + 잠금 + 본인 슬롯 확정 표시
	WidgetRef->ApplyCharacterDisplay(CharacterId);
	WidgetRef->SetConfirmedState();
}

void AFWSelectPlayerController::ApplyConfirmationRejected(const FText& Message)
{
	if (!WidgetRef || Phase != EFWCharacterSelectPhase::ConfirmPending)
	{
		return;
	}

	// 확정 결과가 적용되기 전이므로 서브시스템에는 아무것도 저장하지 않음
	Phase = EFWCharacterSelectPhase::Selecting;
	WidgetRef->SetRequestPending(false);
	WidgetRef->ShowSelectionError(Message.IsEmpty() ? LOCTEXT("Rejected", "선택을 확정하지 못했습니다. 다시 시도해 주세요.") : Message);
	// 로컬 제한 시간이 이미 끝났다면 다음 타이머 갱신에서 자동 확정이 다시 시도됨
}

void AFWSelectPlayerController::ApplyTeamSelectionView(const TArray<FFWTeamSelectPlayerView>& PlayerViews)
{
	// 화면이 닫힌 뒤 늦게 도착한 결과는 무시
	if (!IsValid(WidgetRef))
	{
		return;
	}

	TArray<FFWTeamSelectPlayerView> Views = PlayerViews;
	if (LocalPlayerId != INDEX_NONE)
	{
		// 본인 여부는 ID로 판단
		for (FFWTeamSelectPlayerView& View : Views)
		{
			View.bIsLocalPlayer = (View.PlayerId == LocalPlayerId);
		}
	}
	WidgetRef->ApplyTeamSlots(Views);
}

void AFWSelectPlayerController::ApplyRemainingTime(float Seconds)
{
	if (!IsValid(WidgetRef))
	{
		return;
	}
	const float Clamped = FMath::IsFinite(Seconds) ? FMath::Max(0.f, Seconds) : 0.f;
	WidgetRef->SetRemainingSeconds(FMath::CeilToInt(Clamped));
}

// ---------------------------------------------------------------- 단독 검증용 타이머

void AFWSelectPlayerController::StartLocalSelectionTimer()
{
	const float Duration = FMath::Max(1.f, SelectionDurationSeconds);
	SelectionEndsAtSeconds = GetWorld()->GetTimeSeconds() + Duration;
	ApplyRemainingTime(Duration);
	GetWorldTimerManager().SetTimer(SelectionTimerHandle, this, &AFWSelectPlayerController::UpdateLocalSelectionTimer, .1f, true);
}

void AFWSelectPlayerController::UpdateLocalSelectionTimer()
{
	if (!bUseLocalPreviewFlow || !WidgetRef)
	{
		StopLocalSelectionTimer();
		return;
	}

	const float Remaining = FMath::Max(0.f, static_cast<float>(SelectionEndsAtSeconds - GetWorld()->GetTimeSeconds()));
	ApplyRemainingTime(Remaining);

	if (Phase != EFWCharacterSelectPhase::Selecting || Remaining > 0.f)
	{
		return;
	}

	StopLocalSelectionTimer();

	// 현재 미리보기 ID → 기본 ID 순서로 자동 확정
	FName AutoCharacterId = WidgetRef->GetCurrentCharacterId();
	if (!WidgetRef->IsSelectableCharacter(AutoCharacterId))
	{
		AutoCharacterId = DefaultCharacterId;
	}
	if (!WidgetRef->IsSelectableCharacter(AutoCharacterId))
	{
		WidgetRef->ShowSelectionError(LOCTEXT("AutoConfirmFailed", "자동으로 확정할 캐릭터가 없습니다."));
		return;
	}
	RequestConfirmSelection(AutoCharacterId);
}

void AFWSelectPlayerController::StopLocalSelectionTimer()
{
	GetWorldTimerManager().ClearTimer(SelectionTimerHandle);
}

void AFWSelectPlayerController::ScheduleLocalGameplayStart()
{
	if (LocalConfirmTravelDelaySeconds <= 0.f)
	{
		HandleGameplayStart();
		return;
	}
	GetWorldTimerManager().SetTimer(LocalStartTimerHandle, this, &AFWSelectPlayerController::HandleGameplayStart, LocalConfirmTravelDelaySeconds, false);
}

// ---------------------------------------------------------------- 게임 진입

void AFWSelectPlayerController::HandleGameplayStart()
{
	// 재전달된 시작 이벤트가 이동을 중복 실행하지 않도록
	if (Phase == EFWCharacterSelectPhase::Travelling)
	{
		return;
	}

	const UFWMenuSessionSubsystem* Session = GetMenuSession();
	if (Phase != EFWCharacterSelectPhase::Confirmed || !Session || !Session->IsCharacterSelectionConfirmed())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FWCharacterSelect] Gameplay start ignored: selection is not confirmed (phase %d)."), static_cast<int32>(Phase));
		if (WidgetRef)
		{
			WidgetRef->ShowSelectionError(LOCTEXT("StartWithoutConfirm", "캐릭터 확정 정보가 없어 게임에 진입할 수 없습니다."));
		}
		return;
	}

	BeginGameplayTravel();
}

void AFWSelectPlayerController::BeginGameplayTravel()
{
	if (Phase == EFWCharacterSelectPhase::Travelling)
	{
		return;
	}
	if (!IsGameplayLevelAvailable())
	{
		UE_LOG(LogTemp, Error, TEXT("[FWCharacterSelect] GameplayLevel is unavailable: %s"), *GameplayLevel.GetLongPackageName());
		if (WidgetRef)
		{
			WidgetRef->ShowSelectionError(LOCTEXT("TravelMapMissing", "이동할 게임 맵을 찾을 수 없습니다."));
		}
		return;
	}

	Phase = EFWCharacterSelectPhase::Travelling;
	StopLocalSelectionTimer();
	GetWorldTimerManager().ClearTimer(LocalStartTimerHandle);
	BeginScreenFade(true);
}

void AFWSelectPlayerController::BeginScreenFade(bool bOut)
{
	if (bOut)
	{
		SetInputMode(FInputModeGameOnly());
	}
	bFadeOut = bOut;
	FadeStartedAt = FPlatformTime::Seconds();
	ActiveFadeDuration = FWMenuFade::Duration(bOut ? FadeOutSeconds : FadeInSeconds);
	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);

	if (WidgetRef)
	{
		WidgetRef->SetTransitionOpacity(bOut ? 0.f : 1.f);
	}
	if (ActiveFadeDuration <= 0.f)
	{
		TickScreenFade();
		return;
	}
	GetWorldTimerManager().SetTimer(ScreenFadeTimer, this, &AFWSelectPlayerController::TickScreenFade, .016f, true);
}

void AFWSelectPlayerController::TickScreenFade()
{
	if (!WidgetRef)
	{
		GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
		return;
	}

	const float Progress = ActiveFadeDuration > 0.f
		? static_cast<float>(FMath::Clamp((FPlatformTime::Seconds() - FadeStartedAt) / ActiveFadeDuration, 0.0, 1.0))
		: 1.f;
	WidgetRef->SetTransitionOpacity(bFadeOut ? Progress : 1.f - Progress);
	if (Progress < 1.f)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ScreenFadeTimer);
	if (bFadeOut)
	{
		// 검은 UMG가 한 프레임 표시된 다음 레벨을 엶
		GetWorldTimerManager().SetTimer(LevelTravelTimer, this, &AFWSelectPlayerController::OpenGameplayLevel, .016f, false);
	}
}

void AFWSelectPlayerController::OpenGameplayLevel()
{
	if (Phase != EFWCharacterSelectPhase::Travelling)
	{
		return;
	}

	// 맵 존재 여부와 입력 모드(Game Only)는 BeginGameplayTravel / BeginScreenFade(true)에서 이미 처리
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, GameplayLevel);
}

#undef LOCTEXT_NAMESPACE