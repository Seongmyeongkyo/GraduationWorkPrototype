#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWLoginWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UEditableTextBox;
class UTexture2D;

/** Code-built login screen. Art and panel opacity are configurable in DefaultGame.ini. */
UCLASS(Config = Game)
class FRAMEWORK_API UFWLoginWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFWLoginWidget(const FObjectInitializer& ObjectInitializer);

	TSharedPtr<SWidget> GetInitialFocusWidget() const;
	void ShowStatus(const FText& Message);
	void SetStartingGame();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

	/** Use an imported /Game/... texture asset path, not a filesystem PNG path. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Login|Appearance")
	TSoftObjectPtr<UTexture2D> BackgroundTexture;
	UPROPERTY(Config, EditDefaultsOnly, Category = "Login|Appearance")
	TSoftObjectPtr<UTexture2D> LogoTexture;
	/** 로그인 패널 배경의 불투명도: 0 완전 투명, 1 완전 불투명. 기본 적용값은 DefaultGame.ini에서 수정. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Login|Appearance", meta = (ClampMin = "0", ClampMax = "1"))
	float PanelOpacity = 0.72f;

private:
	void BuildMenu();
	UFUNCTION()
	void OnLoginClicked();
	UFUNCTION()
	void OnSignUpClicked();
	UFUNCTION()
	void OnCredentialsChanged(const FText& Text);
	UFUNCTION()
	void OnUsernameCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UFUNCTION()
	void OnPasswordCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	UFUNCTION()
	void OnStartClicked();
	UFUNCTION()
	void OnControlsClicked();
	UFUNCTION()
	void OnQuitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> StartButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ControlsButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StartLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ControlsLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusLabel;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ControlsPanel;
	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> UsernameInput;
	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> PasswordInput;
	UPROPERTY(Transient)
	TObjectPtr<UButton> LoginButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> SignUpButton;
	bool bStarting = false;
};
