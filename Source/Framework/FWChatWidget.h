// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWChatWidget.generated.h"

class UBorder;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;

/** 채팅 채널 **/
UENUM(BlueprintType)
enum class EFWChatChannel : uint8
{
	All     UMETA(DisplayName = "전체"),
	Team    UMETA(DisplayName = "팀"),
	System  UMETA(DisplayName = "시스템")    // 킬로그·공지 등(수신 전용)
};

/** 전송 : PlayerController가 구독(델리게이트) (로컬 loopback) **/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageSubmitted,
	EFWChatChannel, 
	Channel,
	const FString&,
	MessageText);

/** 입력창이 닫힘 (전송·취소 공통) → PlayerController가 게임 포커스 복구 **/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChatClosedSignature);

/** 로그 한 줄 = 텍스트 위젯 + 도착 시각 (페이드 계산용) **/
USTRUCT()
struct FFWChatLine
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UWidget> TextWidget = nullptr;

	/** 줄이 추가된 시각 (World 시간으로 계산) **/
	double SpawnTime = 0.0;
};

UCLASS(Abstract)
class FRAMEWORK_API UFWChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 입력창 열기. System 채널은 입력 불가 → Team으로 열림 **/
	UFUNCTION(BlueprintCallable, Category = "FW|Chat")
	void OpenChat(EFWChatChannel Channel = EFWChatChannel::Team);

	/** 입력 취소 후 닫기 (쓰던 글자는 버림) **/
	UFUNCTION(BlueprintCallable, Category = "FW|Chat")
	void CloseChat();

	/** 입력창 내용을 전송하고 닫기 (비어 있으면 전송 없이 닫기만) **/
	UFUNCTION(BlueprintCallable, Category = "FW|Chat")
	void SubmitInput();

	/** 로그에 한 줄 추가 — 로컬 loopback **/
	UFUNCTION(BlueprintCallable, Category = "FW|Chat")
	void AddIncomingMessage(EFWChatChannel Channel, const FString& SenderName, const FString& MessageText, bool bIsTeam = true);

	UFUNCTION(BlueprintPure, Category = "FW|Chat")
	bool IsChatOpen() const { return bChatOpen; }

	UFUNCTION(BlueprintPure, Category = "FW|Chat")
	EFWChatChannel GetCurrentChannel() const { return CurrentChannel; }

	/** 전송 확정 시 발동 **/
	UPROPERTY(BlueprintAssignable, Category = "FW|Chat")
	FOnChatMessageSubmitted OnMessageSubmitted;

	/** 입력창이 닫힐 때 발동 (전송·Esc 취소 공통) **/
	UPROPERTY(BlueprintAssignable, Category = "FW|Chat")
	FOnChatClosedSignature OnChatClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// BindWidget
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> MessageScrollBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> InputTextBox;

	/** "[전체]" / "[팀]" 라벨 **/
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ChannelLabel;

	/** 로그 배경 **/
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> LogBackground;

	/** 입력줄 전체(라벨 + 입력창)를 감싸는 컨테이너 **/
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InputBackground;

	/** 로그 최대 줄 수 (초과 시 오래된 줄부터 삭제) **/
	UPROPERTY(EditAnywhere, Category = "FW|Chat", meta = (ClampMin = "1"))
	int32 MaxMessageCount = 50;

	/** 한 번에 보낼 수 있는 최대 글자 수 **/
	UPROPERTY(EditAnywhere, Category = "FW|Chat", meta = (ClampMin = "1"))
	int32 MaxMessageLength = 100;

	UPROPERTY(EditAnywhere, Category = "FW|Chat", meta = (ClampMin = "5"))
	float MessageFontSize = 14.f;

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Color")
	FLinearColor AllChannelColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Color")
	FLinearColor TeamChannelColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Color")
	FLinearColor SystemChannelColor = FLinearColor(1.f, 1.f, 0.0f, 1.f);

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Color")
	FLinearColor TeamNameColor = FLinearColor(0.1f, 0.5f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Color")
	FLinearColor EnemyNameColor = FLinearColor(1.f, 0.1f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Color")
	FLinearColor MessageShadowColor = FLinearColor(0.f, 0.f, 0.f, 1.f);

	/** 닫힌 상태에서 메시지가 선명하게 보이는 시간 **/
	UPROPERTY(EditAnywhere, Category = "FW|Chat|Fade", meta = (ClampMin = "0", Units = "s"))
	float MessageVisibleDuration = 5.f;

	/** 선명하게 보인 뒤 사라지는 데 걸리는 시간 **/
	UPROPERTY(EditAnywhere, Category = "FW|Chat|Fade", meta = (ClampMin = "0.01", Units = "s"))
	float MessageFadeDuration = 2.f;

	/** 페이드 후 최종 불투명도 (0 = 완전히 사라짐) **/
	UPROPERTY(EditAnywhere, Category = "FW|Chat|Fade", meta = (ClampMin = "0", ClampMax = "1"))
	float FadedOpacity = 0.f;

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Fade")
	FLinearColor LogBackgroundOpenColor = FLinearColor(0.f, 0.f, 0.f, 0.45f);

	UPROPERTY(EditAnywhere, Category = "FW|Chat|Fade")
	FLinearColor LogBackgroundClosedColor = FLinearColor(0.f, 0.f, 0.f, 0.f);

private:
	UFUNCTION()
	void HandleInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	void SubmitText(const FString& RawText);
	void ApplyOpenState(bool bOpen);
	void UpdateLineOpacities();
	void MaintainInputFocus();
	void RefreshChannelLabel();
	double GetNow() const;

	FLinearColor GetColorForChannel(EFWChatChannel Channel) const;
	FString GetChannelPrefix(EFWChatChannel Channel) const;

	UPROPERTY()
	TArray<FFWChatLine> ChatLines;

	bool bChatOpen = false;
	EFWChatChannel CurrentChannel = EFWChatChannel::Team;

	/** 마지막으로 입력창을 닫은 시각 — 닫은 직후 로그를 잠깐 다시 보여 주는 데 사용 **/
	double LastCloseTime = -1000.0;
	
};
