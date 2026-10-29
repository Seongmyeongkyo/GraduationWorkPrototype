#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TimerHandle.h"
#include "FWLobbyPlayerController.generated.h"

class UFWLobbyWidget;

/** L_Lobby 전용: 메인/팀 로비 UI와 매칭, 완료 연출, 플레이 맵 이동을 담당합니다. */
UCLASS(Config=Game)
class FRAMEWORK_API AFWLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFWLobbyPlayerController();
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void BeginPrototypeMatch();
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void CancelPrototypeMatch();
	UFUNCTION(BlueprintCallable, Category="Lobby")
	void QuitGame();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** 매칭 완료 후 열 플레이 맵. 패키징 MapsToCook에도 포함해야 합니다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Lobby|Travel")
	TSoftObjectPtr<UWorld> GameplayLevel;
	/** 향후 캐릭터 선택 맵. 현재 비워 두면 GameplayLevel로 이동합니다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Lobby|Travel")
	TSoftObjectPtr<UWorld> CharacterSelectionLevel;
	/** 로컬 테스트 매칭 대기 시간(초). */
	UPROPERTY(Config, EditDefaultsOnly, Category="Lobby|Match", meta=(ClampMin="0.1",ClampMax="60"))
	float PrototypeMatchDelaySeconds = 5.f;
	/** 완료 배너를 온전히 표시하는 시간(초). 이후 페이드아웃합니다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Lobby|Match", meta=(ClampMin="0.1",ClampMax="10"))
	float MatchCompleteDisplaySeconds = 3.f;
	/** 로비에 들어올 때 검은 화면이 사라지는 시간(초). 0이면 즉시 표시. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Lobby|Fade", meta=(ClampMin="0",ClampMax="2"))
	float FadeInSeconds = .25f;
	/** 플레이 맵 이동 직전 화면이 검게 되는 시간(초). */
	UPROPERTY(Config, EditDefaultsOnly, Category="Lobby|Fade", meta=(ClampMin="0",ClampMax="2"))
	float FadeOutSeconds = .25f;
private:
	UPROPERTY(Transient)
	TObjectPtr<UFWLobbyWidget> LobbyWidget;
	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> PendingDestination;
	void RestoreMenuInput();
	void TickPrototypeMatch();
	void CompletePrototypeMatch();
	void ContinueAfterMatchComplete();
	void BeginScreenFade(bool bOut);
	void TickScreenFade();
	void OpenPendingLevel();
	FTimerHandle PrototypeMatchTimer;
	FTimerHandle MatchCompleteTimer;
	FTimerHandle ScreenFadeTimer;
	FTimerHandle LevelTravelTimer;
	double MatchStartedAt = 0;
	double ActiveMatchDelay = 5;
	double FadeStartedAt = 0;
	float ActiveFadeDuration = .25f;
	bool bMatching = false;
	bool bShowingMatchComplete = false;
	bool bStartingGame = false;
	bool bFadeOut = false;
};
