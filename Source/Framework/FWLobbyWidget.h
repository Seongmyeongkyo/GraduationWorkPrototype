#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "FWLobbyWidget.generated.h"

class UButton;
class UCanvasPanel;
class UCanvasPanelSlot;
class UTextBlock;
class UTexture2D;
class UWidget;
class UImage;
class UEditableTextBox;

/** 메인 로비와 팀 로비. 승인된 시안의 PNG 스킨과 UMG 문자를 분리한다. */
UCLASS(Config = Game)
class FRAMEWORK_API UFWLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFWLobbyWidget(const FObjectInitializer& ObjectInitializer);
	TSharedPtr<SWidget> GetInitialFocusWidget() const;
	void SetSearching(bool bInSearching, double ElapsedSeconds = 0.0);
	void ShowStatus(const FText& Message);
	void SetTravelling();
	void ShowMatchComplete();
	void HideMatchComplete();
	void SetLocalNickname(const FString& Nickname);
	bool IsInTeamLobby() const { return bTeamLobby; }
	bool IsInvitationOpen() const { return bInviteOpen; }
	bool IsSearching() const { return bSearching; }
	bool IsMatchCompleteVisible() const { return bShowingMatchComplete; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

	/** 로비 전용 원본 PNG 경로(ProjectDir 상대). 비우면 아래 imported texture 사용. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Appearance")
	FString BackgroundImagePath;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Appearance")
	TSoftObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|Appearance")
	TSoftObjectPtr<UTexture2D> LogoTexture;

	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|MatchComplete")
	FString MatchCompleteBackgroundImagePath;
	UPROPERTY(Config, EditDefaultsOnly, Category = "Lobby|MatchComplete")
	FString MatchCompleteTextImagePath;

private:
	void BuildLobby();
	UCanvasPanelSlot* Place(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Size);
	UTexture2D* LoadComponent(FName Name);
	FSlateBrush MakeSkin(FName Name);
	void StyleButton(UButton* Button, const FString& Family);
	void BuildMainAndTeam(UCanvasPanel* Canvas);
	void BuildInvitation();
	void RefreshScreen();
	void CloseInvitation();
	UTextBlock* MakeText(FName Name, const FText& Value, int32 Size, FLinearColor Color, bool bBold = false);
	UImage* MakeImage(UCanvasPanel* Canvas, FName Name, UTexture2D* Texture, FVector2D Position, FVector2D Size);
	UButton* MakeButton(UCanvasPanel* Canvas, FName Name, const FText& Value, FVector2D Position, FVector2D Size, const TCHAR* Family, int32 FontSize, UTextBlock*& Label);

	UFUNCTION()
	void OnMatchClicked();
	UFUNCTION()
	void OnGameStartClicked();
	UFUNCTION()
	void OnBackClicked();
	UFUNCTION()
	void OnInviteClicked();
	UFUNCTION()
	void OnInviteCancel();
	UFUNCTION()
	void OnInviteSubmit();
	UFUNCTION()
	void OnInviteTextChanged(const FText& Text);
	UFUNCTION()
	void OnInviteCommitted(const FText& Text, ETextCommit::Type Method);
	UFUNCTION()
	void OnCharacterClicked();
	UFUNCTION()
	void OnQuitClicked();

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTexture2D>> ComponentTextures;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> LoadedBackgroundTexture;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MatchCompleteBackground;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MatchCompleteText;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> LobbyContent;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> MatchCompleteOverlay;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> ButtonOrnaments;
	UPROPERTY(Transient)
	TObjectPtr<UButton> MatchButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> CharacterButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> GameStartButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BackButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MatchLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CharacterLabel;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> MainContent;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> TeamContent;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> SearchStatus;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ElapsedLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NoteLabel;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> TeamGroup;
	UPROPERTY(Transient)
	TObjectPtr<UImage> LobbyLogo;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MainNickname;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LeaderNickname;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> InviteButtons;
	UPROPERTY(Transient)
	TObjectPtr<UImage> LeaderCard;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> InvitationOverlay;
	UPROPERTY(Transient)
	TObjectPtr<UButton> InviteSubmitButton;
	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> InviteInput;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InviteStatus;
	FString LocalNickname = TEXT("닉네임");

	bool bSearching = false;
	bool bTravelling = false;
	bool bTeamLobby = false;
	bool bInviteOpen = false;
	bool bShowingMatchComplete = false;
};
