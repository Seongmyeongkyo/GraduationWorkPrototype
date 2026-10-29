#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TimerHandle.h"
#include "FWLoginPlayerController.generated.h"

class UFWLoginWidget;
class UFWLobbyWidget;

UCLASS(Config = Game)
class FRAMEWORK_API AFWLoginPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFWLoginPlayerController();

	/** 기존 로컬 플레이 진입점. 이제 즉시 맵 이동 대신 로비를 표시한다. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame();
	/** 계정 인증 없이 화면 흐름만 확인하는 명시적인 로컬 테스트 로그인. */
	bool TryPrototypeLogin(const FString& Nickname);
	bool IsPrototypeLoginEnabled() const { return bEnablePrototypeLogin; }

	/** 서버 요청 없이 로컬 프로토타입 대기를 시작/취소한다. */
	UFUNCTION(BlueprintCallable, Category = "Menu|Lobby")
	void BeginPrototypeMatch();

	UFUNCTION(BlueprintCallable, Category = "Menu|Lobby")
	void CancelPrototypeMatch();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Also include the destination in ProjectPackagingSettings.MapsToCook. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Menu")
	TSoftObjectPtr<UWorld> GameplayLevel;

	/** 미구현 상태에서는 비워 둔다. 설정 시 매칭 완료 후 이 맵으로 이동한다. MapsToCook에도 추가 필요. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Menu|Lobby")
	TSoftObjectPtr<UWorld> CharacterSelectionLevel;

	/** 임시 매칭 대기 시간(초). 실제 서버 매칭 기능과는 별개이다. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Menu|Lobby", meta = (ClampMin = "0.1", ClampMax = "60"))
	float PrototypeMatchDelaySeconds = 5.f;

	/** 매칭 완료 배너를 표시한 뒤 다음 화면으로 이동하기까지의 대기 시간(초). */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Menu|Lobby", meta = (ClampMin = "0.1", ClampMax = "10"))
	float MatchCompleteDisplaySeconds = 3.f;

	/** True: 테스트 로그인으로 로비 진입. 실제 계정 서버를 붙일 때 False로 전환합니다. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Menu|Prototype")
	bool bEnablePrototypeLogin = true;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFWLoginWidget> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UFWLobbyWidget> LobbyWidget;

	void TickPrototypeMatch();
	void CompletePrototypeMatch();
	void ContinueAfterMatchComplete();
	void RestoreMenuInput();
	FTimerHandle PrototypeMatchTimer;
	FTimerHandle MatchCompleteTimer;
	double MatchStartedAt = 0.0;
	double ActiveMatchDelay = 5.0;
	bool bMatching = false;
	bool bShowingMatchComplete = false;

	bool bStartingGame = false;
	FString LocalPreviewNickname = TEXT("닉네임");
};
