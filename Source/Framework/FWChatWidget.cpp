// Fill out your copyright notice in the Description page of Project Settings.

#include "FWChatWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/EditableTextBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

// ============================================================================
// 생명주기
// ============================================================================
void UFWChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InputTextBox) {
		// 위젯이 다시 부착돼 NativeConstruct가 또 불려도 중복 바인딩되지 않도록 Unique
		InputTextBox->OnTextCommitted.AddUniqueDynamic(this, &UFWChatWidget::HandleInputCommitted);
	}

	ApplyOpenState(false);   // 시작은 닫힌 상태
}

void UFWChatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateLineOpacities();   // 닫힌 상태 페이드
	MaintainInputFocus();    // 열린 상태 입력창 포커스 유지
}

FReply UFWChatWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Preview 단계 = 입력창(자식)보다 먼저 키를 받는 단계
	if (bChatOpen) {
		const FKey PressedKey = InKeyEvent.GetKey();

		// Esc: 입력 취소. Handled로 끝내야 에디터(PIE 종료 단축키)까지 전달되지 않음
		if (PressedKey == EKeys::Escape) {
			CloseChat();
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

// ============================================================================
// 열기 / 닫기 / 전송
// ============================================================================
void UFWChatWidget::OpenChat(EFWChatChannel Channel)
{
	if (bChatOpen || !InputTextBox) {
		return;
	}

	// System은 수신 전용 채널/ 기본 입력은 Team
	CurrentChannel = (Channel == EFWChatChannel::System) ? EFWChatChannel::Team : Channel;

	ApplyOpenState(true);

	InputTextBox->SetText(FText::GetEmpty());
	InputTextBox->SetKeyboardFocus();

	if (MessageScrollBox) {
		MessageScrollBox->ScrollToEnd();
	}
}

void UFWChatWidget::CloseChat()
{
	if (!bChatOpen) {
		return;
	}

	LastCloseTime = GetNow();

	if (InputTextBox) {
		InputTextBox->SetText(FText::GetEmpty());
	}

	ApplyOpenState(false);

	if (MessageScrollBox) {
		MessageScrollBox->ScrollToEnd();
	}

	// 전송·취소 공통 → PlayerController가 게임 포커스 복구
	OnChatClosed.Broadcast();
}

void UFWChatWidget::SubmitInput()
{
	if (!bChatOpen || !InputTextBox) {
		return;
	}

	SubmitText(InputTextBox->GetText().ToString());
}

void UFWChatWidget::HandleInputCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// Enter만 전송으로 처리.
	// OnUserMovedFocus(게임 화면 클릭) / OnCleared(포커스 해제) / Default 는 무시 → 쓰던 글자 유지
	if (CommitMethod != ETextCommit::OnEnter) {
		return;
	}

	SubmitText(Text.ToString());
}

void UFWChatWidget::SubmitText(const FString& RawText)
{
	if (!bChatOpen) {
		return;
	}

	// 앞뒤 공백 제거 + 길이 제한
	const FString CleanText = RawText.TrimStartAndEnd().Left(MaxMessageLength);

	if (!CleanText.IsEmpty()) {
		OnMessageSubmitted.Broadcast(CurrentChannel, CleanText);
	}

	CloseChat();   // 빈 입력이면 전송 없이 닫기
}

// ============================================================================
// 메시지 표시
// ============================================================================
void UFWChatWidget::AddIncomingMessage(EFWChatChannel Channel, const FString& SenderName, const FString& MessageText)
{
	if (!MessageScrollBox || !WidgetTree) {
		return;
	}

	if (GetVisibility() == ESlateVisibility::Collapsed) {
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 입력창을 열고 위로 스크롤해 지난 로그를 읽는 중이면 바닥으로 끌어내리지 않음
	const bool bStickToEnd = !bChatOpen
		|| MessageScrollBox->GetScrollOffset() >= MessageScrollBox->GetScrollOffsetOfEnd() - 1.f;

	UTextBlock* NewLine = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	if (!NewLine) {
		return;
	}

	const FString Formatted = (Channel == EFWChatChannel::System)
		? FString::Printf(TEXT("%s %s"), *GetChannelPrefix(Channel), *MessageText)
		: FString::Printf(TEXT("%s %s: %s"), *GetChannelPrefix(Channel), *SenderName, *MessageText);

	NewLine->SetText(FText::FromString(Formatted));
	NewLine->SetColorAndOpacity(FSlateColor(GetColorForChannel(Channel)));
	NewLine->SetAutoWrapText(true);                            // 긴 문장 자동 줄바꿈
	NewLine->SetShadowOffset(FVector2D(1.f, 1.f));
	NewLine->SetShadowColorAndOpacity(MessageShadowColor);

	FSlateFontInfo FontInfo = NewLine->GetFont();
	FontInfo.Size = MessageFontSize;
	NewLine->SetFont(FontInfo);

	MessageScrollBox->AddChild(NewLine);

	FFWChatLine NewEntry;
	NewEntry.TextWidget = NewLine;
	NewEntry.SpawnTime = GetNow();
	ChatLines.Add(NewEntry);

	// 최대 줄 수 초과시 가장 오래된 줄부터 삭제
	while (ChatLines.Num() > MaxMessageCount) {
		if (ChatLines[0].TextWidget) {
			ChatLines[0].TextWidget->RemoveFromParent();
		}
		ChatLines.RemoveAt(0);
	}

	if (bStickToEnd) {
		MessageScrollBox->ScrollToEnd();
	}
}

void UFWChatWidget::UpdateLineOpacities()
{
	const double Now = GetNow();
	const float SafeFadeDuration = FMath::Max(MessageFadeDuration, 0.01f);
	bool bAnyLineVisible = false;

	for (const FFWChatLine& Line : ChatLines) {
		if (!Line.TextWidget) {
			continue;
		}

		float TargetOpacity = 1.f;   // 열린 상태 = 전부 선명

		if (!bChatOpen) {
			// 도착시간 / 창을 닫은 시각 중 늦은 쪽부터 카운트
			// 창을 닫으면 최근 로그가 잠깐 다시 보였다가 사라짐
			const double VisibleFrom = FMath::Max(Line.SpawnTime, LastCloseTime);
			const float Age = static_cast<float>(Now - VisibleFrom);
			const float FadeAlpha = FMath::Clamp((Age - MessageVisibleDuration) / SafeFadeDuration, 0.f, 1.f);
			TargetOpacity = FMath::Lerp(1.f, FadedOpacity, FadeAlpha);
		}

		if (TargetOpacity > 0.f) {
			bAnyLineVisible = true;
		}

		// 값이 바뀔 때만 갱신 (불필요한 다시 그리기 방지)
		if (!FMath::IsNearlyEqual(Line.TextWidget->GetRenderOpacity(), TargetOpacity)) {
			Line.TextWidget->SetRenderOpacity(TargetOpacity);
		}
	}

	if(!bChatOpen && !bAnyLineVisible) {
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

// ============================================================================
// 상태 / 포커스
// ============================================================================
void UFWChatWidget::ApplyOpenState(bool bOpen)
{
	bChatOpen = bOpen;

	// 열림: 루트는 클릭 통과, 자식(로그·입력창)만 반응
	// 닫힘: 전체 클릭 통과 : 채팅 영역 위를 클릭해도 게임이 그대로 받음
	SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::HitTestInvisible);

	if (InputBackground) {
		InputBackground->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (InputTextBox) {
		InputTextBox->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	if (ChannelLabel) {
		ChannelLabel->SetVisibility(bOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (LogBackground) {
		LogBackground->SetBrushColor(bOpen ? LogBackgroundOpenColor : LogBackgroundClosedColor);
	}
	if (MessageScrollBox) {
		// 스크롤바도 Hidden으로 자리 유지 → 줄바꿈 폭이 바뀌지 않음 (열려 있어도 넘칠 때만 표시됨)
		MessageScrollBox->SetScrollBarVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

		// ScrollBox는 내용이 스크롤되면 위·아래 가장자리에 그림자를 그림
		// 닫혀 있는 동안 가장자리 그림자를 숨김
		FScrollBoxStyle ScrollStyle = MessageScrollBox->GetWidgetStyle();
		const FSlateColor ShadowTint(FLinearColor(1.f, 1.f, 1.f, bOpen ? 1.f : 0.f));
		ScrollStyle.TopShadowBrush.TintColor = ShadowTint;
		ScrollStyle.BottomShadowBrush.TintColor = ShadowTint;
		MessageScrollBox->SetWidgetStyle(ScrollStyle);
	}

	if (bOpen) {
		RefreshChannelLabel();
	}

	UpdateLineOpacities();   // 한 프레임 늦지 않게 즉시 반영
}

void UFWChatWidget::MaintainInputFocus()
{
	if (!bChatOpen || !InputTextBox) {
		return;
	}

	// 이미 포커스가 있으면 아무것도 하지 않음
	// EditableTextBox는 실제 포커스를 내부 SEditableText에 넘기므로 자손 포커스까지 확인
	// 여기서 잘못 판단해 매 틱 SetKeyboardFocus를 부르면 한글 조합이 계속 끊김.
	if (InputTextBox->HasKeyboardFocus() || InputTextBox->HasFocusedDescendants()) {
		return;
	}

	// 창이 비활성(Alt+Tab 등)이면 되찾지 않음
	if (!FSlateApplication::IsInitialized() || !FSlateApplication::Get().IsActive()) {
		return;
	}

	// 마우스 버튼을 누르고 있는 동안은 게임이 입력을 쓰는 중 → 뗀 뒤에 되찾음
	// (누르는 중에 뺏으면 뷰포트 FocusLost → FlushPressedKeys → 우클릭 연속 이동이 끊김)
	if (const APlayerController* OwningPC = GetOwningPlayer()) {
		if (OwningPC->IsInputKeyDown(EKeys::LeftMouseButton) || OwningPC->IsInputKeyDown(EKeys::RightMouseButton)) {
			return;
		}
	}

	InputTextBox->SetKeyboardFocus();
}

void UFWChatWidget::RefreshChannelLabel()
{
	if (!ChannelLabel) {
		return;
	}

	ChannelLabel->SetText(FText::FromString(GetChannelPrefix(CurrentChannel)));
	ChannelLabel->SetColorAndOpacity(FSlateColor(GetColorForChannel(CurrentChannel)));
}

// ============================================================================
// 유틸
// ============================================================================
double UFWChatWidget::GetNow() const
{
	// 게임 시간이 아닌 실제 시간
	const UWorld* CurrentWorld = GetWorld();
	return CurrentWorld ? CurrentWorld->GetRealTimeSeconds() : 0.0;
}

FLinearColor UFWChatWidget::GetColorForChannel(EFWChatChannel Channel) const
{
	switch (Channel) {
	case EFWChatChannel::All:   return AllChannelColor;
	case EFWChatChannel::System: return SystemChannelColor;
	default:                     return TeamChannelColor;
	}
}

FString UFWChatWidget::GetChannelPrefix(EFWChatChannel Channel) const
{
	switch (Channel) {
	case EFWChatChannel::All:   return TEXT("[전체]");
	case EFWChatChannel::System: return TEXT("[시스템]");
	default:                     return TEXT("[팀]");
	}
}