#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TimerHandle.h"
#include "FWLoginPlayerController.generated.h"

class UFWLoginWidget;

/** L_LoginScreen 전용. 로그인 UI와 로비 레벨 진입만 담당합니다. */
UCLASS(Config=Game)
class FRAMEWORK_API AFWLoginPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFWLoginPlayerController();
	/** 기존 로컬 플레이 진입점. L_Lobby를 열고 로비 컨트롤러에 역할을 넘깁니다. */
	UFUNCTION(BlueprintCallable, Category="Login")
	void StartGame();
	bool TryPrototypeLogin(const FString& Nickname);
	bool IsPrototypeLoginEnabled() const { return bEnablePrototypeLogin; }
	UFUNCTION(BlueprintCallable, Category="Login")
	void QuitGame();
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	/** 로그인 완료 후 열 로비 레벨. MapsToCook에도 포함해야 합니다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Login|Travel")
	TSoftObjectPtr<UWorld> LobbyLevel;
	/** 계정 인증 대신 표시 이름만 넘기는 로컬 테스트 로그인. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Login|Prototype")
	bool bEnablePrototypeLogin = true;
	/** 로그인 화면을 처음 표시할 때의 페이드인 시간(초). */
	UPROPERTY(Config, EditDefaultsOnly, Category="Login|Fade", meta=(ClampMin="0",ClampMax="2"))
	float FadeInSeconds = .25f;
	/** 로비로 이동하기 전 페이드아웃 시간(초). 0이면 즉시 전환합니다. */
	UPROPERTY(Config, EditDefaultsOnly, Category="Login|Fade", meta=(ClampMin="0",ClampMax="2"))
	float FadeOutSeconds = .25f;
private:
	UPROPERTY(Transient)
	TObjectPtr<UFWLoginWidget> MenuWidget;
	void ShowLoginScreen(const FText& Message);
	void RestoreLoginInput();
	void BeginScreenFade(bool bOut);
	void TickScreenFade();
	void OpenLobbyLevel();
	FTimerHandle ScreenFadeTimer;
	FTimerHandle LevelTravelTimer;
	double FadeStartedAt = 0;
	float ActiveFadeDuration = .25f;
	bool bFadeOut = false;
	bool bStartingGame = false;
};
