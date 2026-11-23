#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TimerHandle.h"
#include "FWCharacterSelectTypes.h"
#include "FWSelectPlayerController.generated.h"

class UFWCharacterSelectWidget;
class UFWMenuSessionSubsystem;

/**
 * L_CharacterSelect 전용 (BP_PC_CharacterSelect의 부모).
 * 화면 초기화, 진행 상태, 확정 요청/결과 적용, Nature 맵 이동을 담당한다.
 * 서버 통신은 하지 않는다. 연동 코드는 OnSelectionConfirmRequested를 받고 Apply* 함수를 호출하면 된다.
 */
UCLASS(Config=Game)
class FRAMEWORK_API AFWSelectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFWSelectPlayerController();

	/** 입력 검사 후 확정 요청. 단독 검증 모드면 바로 확정, 아니면 OnSelectionConfirmRequested 발송 후 대기 **/
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void RequestConfirmSelection(FName CharacterId);

	/** 확정 결과 저장 및 UI 잠금 (맵 이동은 하지 않음) **/
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void ApplyConfirmedSelection(FName CharacterId);

	/** 확정 요청 거절: 대기 해제, 입력 복구, 안내 **/
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void ApplyConfirmationRejected(const FText& Message);

	/** 팀원 슬롯 갱신 **/
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void ApplyTeamSelectionView(const TArray<FFWTeamSelectPlayerView>& PlayerViews);

	/** 전달받은 남은 시간 표시 (표시만 함, 0이 되어도 확정/진입하지 않음) **/
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void ApplyRemainingTime(float Seconds);

	/** 게임 진입 알림: 확정 정보 확인 후 Nature 이동 **/
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void HandleGameplayStart();

	/** 팀원 목록에서 본인을 구분할 ID (연동 코드가 알게 되면 지정) **/
	UFUNCTION(BlueprintCallable, Category = "CharacterSelect")
	void SetLocalPlayerId(int32 InLocalPlayerId) { LocalPlayerId = InLocalPlayerId; }

	/** 실제 연동 모드에서 확정 요청을 기존 연동 코드로 넘기는 지점 **/
	UPROPERTY(BlueprintAssignable, Category = "CharacterSelect")
	FFWCharacterIdSignature OnSelectionConfirmRequested;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** WBP_CharacterSelect **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect")
	TSubclassOf<UFWCharacterSelectWidget> CharacterSelectWidgetClass;

	/** 처음 미리보기할 캐릭터 **/
	UPROPERTY(Config, EditDefaultsOnly, Category = "CharacterSelect")
	FName DefaultCharacterId = TEXT("Luna");

	/** 단독 검증용 흐름 (로컬 30초 타이머, 시간 초과 자동 확정, 요청 즉시 확정). 실제 연동 시 False **/
	UPROPERTY(Config, EditDefaultsOnly, Category = "CharacterSelect|LocalPreview")
	bool bUseLocalPreviewFlow = true;

	/** 단독 검증용 제한 시간(초) **/
	UPROPERTY(Config, EditDefaultsOnly, Category = "CharacterSelect|LocalPreview", meta = (ClampMin = "1.0", EditCondition = "bUseLocalPreviewFlow"))
	float SelectionDurationSeconds = 30.f;

	/** 단독 검증 모드에서 확정 표시를 보여 준 뒤 Nature로 이동하기까지 대기(초) **/
	UPROPERTY(Config, EditDefaultsOnly, Category = "CharacterSelect|LocalPreview", meta = (ClampMin = "0.0", EditCondition = "bUseLocalPreviewFlow"))
	float LocalConfirmTravelDelaySeconds = 0.8f;

	/** 확정 후 열 플레이 맵 (로비 컨트롤러 값이 자동 복사되지 않으므로 여기서 지정) **/
	UPROPERTY(Config, EditDefaultsOnly, Category = "CharacterSelect|Travel")
	TSoftObjectPtr<UWorld> GameplayLevel;

	/** 선택 맵에 들어올 때 검은 화면이 사라지는 시간(초) **/
	UPROPERTY(Config, EditDefaultsOnly, Category = "CharacterSelect|Fade", meta = (ClampMin = "0", ClampMax = "2"))
	float FadeInSeconds = .25f;

	/** Nature 이동 직전 화면이 검게 되는 시간(초) **/
	UPROPERTY(Config, EditDefaultsOnly, Category = "CharacterSelect|Fade", meta = (ClampMin = "0", ClampMax = "2"))
	float FadeOutSeconds = .25f;

private:
	UFWMenuSessionSubsystem* GetMenuSession() const;
	bool IsGameplayLevelAvailable() const;
	void RestoreSelectInput();

	void StartLocalSelectionTimer();
	void UpdateLocalSelectionTimer();
	void StopLocalSelectionTimer();
	void ScheduleLocalGameplayStart();

	void BeginGameplayTravel();
	void BeginScreenFade(bool bOut);
	void TickScreenFade();
	void OpenGameplayLevel();

	UPROPERTY(Transient)
	TObjectPtr<UFWCharacterSelectWidget> WidgetRef;

	EFWCharacterSelectPhase Phase = EFWCharacterSelectPhase::Initializing;
	int32 LocalPlayerId = INDEX_NONE;

	FTimerHandle SelectionTimerHandle;
	FTimerHandle LocalStartTimerHandle;
	FTimerHandle ScreenFadeTimer;
	FTimerHandle LevelTravelTimer;

	double SelectionEndsAtSeconds = 0.0;
	double FadeStartedAt = 0.0;
	float ActiveFadeDuration = .25f;
	bool bFadeOut = false;
};