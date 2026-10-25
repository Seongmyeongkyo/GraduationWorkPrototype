#include "FWLoginWidget.h"
#include "FWLoginPlayerController.h"
#include "FWLoginWindLayer.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "FWLogin"

// 로그인 화면 크기 조절표. 단위는 1920x1080 기준 UI 단위이며 실제 창에 맞춰 비례 조절됩니다.
// 패널 높이는 아래 내용과 여백을 합산해 자동 결정됩니다. 로고/입력칸을 키우면 패널도 커집니다.
namespace LoginLayout
{
	constexpr float DesignWidth = 1920.f;				// 화면 배치의 기준 가로 크기
	constexpr float DesignHeight = 1080.f;				// 화면 배치의 기준 세로 크기
	constexpr float LeftMargin = 144.f;					// 화면 왼쪽과 로그인 패널 사이 간격 (작을수록 왼쪽)
	constexpr float CardWidth = 528.f;					// 로그인 패널 전체 가로 크기
	constexpr float CardVerticalOffset = 0.f;			// 화면 중앙 기준 패널 위/아래 이동 (음수는 위)
	constexpr float CardCornerRadius = 22.f;			// 로그인 패널 모서리 둥글기
	const FMargin CardPadding(48.f, 34.f, 48.f, 31.f);	// 패널 안쪽 여백: 왼쪽, 위, 오른쪽, 아래
	constexpr float LogoHeight = 206.f;					// 로고 표시 영역 높이 (가로는 패널 안쪽 너비, 원본 비율 유지)
	constexpr float LogoBottomGap = 24.f;				// 로고와 아이디 입력칸 사이 간격
	constexpr int32 FallbackLogoFont = 58;				// 로고 이미지 로드 실패 시 Close IN 대체 글자 크기
	constexpr float InputHeight = 65.f;					// 아이디/비밀번호 입력칸 각각의 높이
	constexpr int32 InputFont = 21;						// 입력 글자와 아이디/비밀번호 안내 글자 크기
	const FMargin InputPadding(19.f, 14.f);				// 입력칸 안쪽 가로/세로 여백
	constexpr float InputGap = 14.f;					// 아이디 입력칸과 비밀번호 입력칸 사이 간격
	constexpr float PasswordBottomGap = 24.f;			// 비밀번호 입력칸과 로그인 버튼 사이 간격
	constexpr float ControlCornerRadius = 10.f;			// 입력칸과 버튼의 모서리 둥글기
	constexpr float FocusOutlineWidth = 2.f;			// 선택된 입력칸의 테두리 두께
	constexpr int32 PrimaryFont = 22;					// 로그인 버튼 글자 크기
	constexpr int32 SecondaryFont = 17;					// 회원가입/로컬 플레이 글자 크기
	constexpr float ButtonHorizontalPadding = 14.f;		// 버튼 안쪽 좌우 여백
	constexpr float PrimaryVerticalPadding = 17.f;		// 로그인 버튼 안쪽 상하 여백 (버튼 높이에 영향)
	constexpr float SecondaryVerticalPadding = 10.f;	// 회원가입/로컬 플레이 버튼 안쪽 상하 여백
	constexpr float ButtonBottomGap = 12.f;				// 로그인 버튼 및 회원가입 행 아래 간격
	constexpr int32 StatusFont = 15;					// 계정 서비스 상태 안내 글자 크기
	constexpr float StatusHeight = 50.f;				// 상태 안내 영역 높이 (두 줄 안내 공간)
	constexpr float UtilityBottomMargin = 32.f;			// 하단 조작 방법/게임 종료 버튼과 화면 아래 간격
	constexpr float UtilityButtonWidth = 184.f;			// 하단 버튼 각각의 가로 크기
	constexpr float UtilityButtonHeight = 58.f;			// 하단 버튼 각각의 세로 크기
	constexpr float UtilityButtonGap = 12.f;			// 하단 두 버튼 사이 간격
	constexpr int32 UtilityFont = 19;					// 하단 조작 방법/게임 종료 글자 크기
	constexpr float HelpPanelWidth = 520.f;				// 펼친 조작 안내 박스 가로 크기
	constexpr float HelpPanelGap = 24.f;				// 로그인 패널 오른쪽과 조작 안내 박스 사이 간격
	constexpr float HelpPadding = 20.f;					// 조작 안내 박스 안쪽 여백
	constexpr int32 HelpFont = 16;						// 조작 안내 본문 글자 크기
}

UFWLoginWidget::UFWLoginWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BackgroundTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Login/T_LoginBackground_Forest_v3.T_LoginBackground_Forest_v3")));
	LogoTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Login/T_Logo_Mix_v7.T_Logo_Mix_v7")));
}

TSharedRef<SWidget> UFWLoginWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildMenu();
	}
	return Super::RebuildWidget();
}

void UFWLoginWidget::BuildMenu()
{
	using namespace LoginLayout;
	const FLinearColor Cream = FLinearColor::FromSRGBColor(FColor(242, 219, 169));
	const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(202, 207, 202));
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(30, 39, 37));
	auto MakeText = [this](const FText& Text, int32 Size, const FLinearColor& Color, bool bBold = false)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
		Label->SetColorAndOpacity(FSlateColor(Color));
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(true);
		return Label;
	};

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	Root->SetClipping(EWidgetClipping::ClipToBounds);
	WidgetTree->RootWidget = Root;
	// 원본 배경 이미지는 고정. 움직임은 아래 별도 투명 잎 레이어에만 적용합니다.
	UScaleBox* BackgroundCover = WidgetTree->ConstructWidget<UScaleBox>();
	BackgroundCover->SetStretch(EStretch::ScaleToFill);
	BackgroundCover->SetVisibility(ESlateVisibility::HitTestInvisible);
	UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(BackgroundCover);
	BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
	BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LoginBackground"));
	Background->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UTexture2D* Texture = BackgroundTexture.LoadSynchronous())
	{
		Background->SetBrushFromTexture(Texture, true);
	}
	else
	{
		Background->SetColorAndOpacity(FLinearColor(0.02f, 0.03f, 0.025f));
		UE_LOG(LogTemp, Warning, TEXT("[FWLogin] Background texture unavailable: %s"), *BackgroundTexture.ToString());
	}
	BackgroundCover->SetContent(Background);

	// 승인된 HTML 미리보기의 잎 레이어. 배경 이미지나 로그인 패널을 움직이지 않습니다.
	UFWLoginWindLayer* Wind = WidgetTree->ConstructWidget<UFWLoginWindLayer>(UFWLoginWindLayer::StaticClass(), TEXT("LoginWindLayer"));
	Wind->SetVisibility(ESlateVisibility::HitTestInvisible);
	Wind->SetClipping(EWidgetClipping::ClipToBounds);
	UOverlaySlot* WindSlot = Root->AddChildToOverlay(Wind);
	WindSlot->SetHorizontalAlignment(HAlign_Fill);
	WindSlot->SetVerticalAlignment(VAlign_Fill);

	// Keep the card's proportions and safe margins at different viewport sizes.
	UScaleBox* UIScale = WidgetTree->ConstructWidget<UScaleBox>();
	UIScale->SetStretch(EStretch::ScaleToFit);
	UOverlaySlot* UISlot = Root->AddChildToOverlay(UIScale);
	UISlot->SetHorizontalAlignment(HAlign_Fill);
	UISlot->SetVerticalAlignment(VAlign_Fill);
	USizeBox* DesignSize = WidgetTree->ConstructWidget<USizeBox>();
	DesignSize->SetWidthOverride(DesignWidth);
	DesignSize->SetHeightOverride(DesignHeight);
	//UIScale->SetContent(DesignSize);	
	if (UScaleBoxSlot* DesignSlot = Cast<UScaleBoxSlot>(UIScale->SetContent(DesignSize)))
	{
		DesignSlot->SetHorizontalAlignment(HAlign_Left);
	}
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	DesignSize->SetContent(Canvas);
	USizeBox* CardWidthBox = WidgetTree->ConstructWidget<USizeBox>();
	CardWidthBox->SetWidthOverride(CardWidth);
	UCanvasPanelSlot* CardSlot = Canvas->AddChildToCanvas(CardWidthBox);
	CardSlot->SetAnchors(FAnchors(0.f, 0.5f));
	CardSlot->SetAlignment(FVector2D(0.f, 0.5f));
	CardSlot->SetPosition(FVector2D(LeftMargin, CardVerticalOffset));
	CardSlot->SetAutoSize(true);
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LoginPanel"));
	Panel->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, CardCornerRadius));
	Panel->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(PanelOpacity, 0.f, 1.f)));
	Panel->SetPadding(CardPadding);
	CardWidthBox->SetContent(Panel);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->SetContent(Column);
	auto AddRow = [Column](UWidget* Widget, float BottomPadding)
	{
		UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget);
		Slot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPadding));
		Slot->SetHorizontalAlignment(HAlign_Fill);
	};

	USizeBox* LogoSize = WidgetTree->ConstructWidget<USizeBox>();
	LogoSize->SetHeightOverride(LogoHeight);
	UScaleBox* LogoScale = WidgetTree->ConstructWidget<UScaleBox>();
	LogoScale->SetStretch(EStretch::ScaleToFit);
	LogoSize->SetContent(LogoScale);
	UImage* Logo = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LoginLogo"));
	if (UTexture2D* Texture = LogoTexture.LoadSynchronous())
	{
		Logo->SetBrushFromTexture(Texture, true);
		LogoScale->SetContent(Logo);
	}
	else
	{
		LogoScale->SetContent(MakeText(LOCTEXT("Title", "Close IN"), FallbackLogoFont, Cream, true));
		UE_LOG(LogTemp, Warning, TEXT("[FWLogin] Logo texture unavailable: %s"), *LogoTexture.ToString());
	}
	AddRow(LogoSize, LogoBottomGap);

	auto AddInput = [this, &AddRow, &Ink](FName Name, FText Hint, bool bPassword)
	{
		UEditableTextBox* Input = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), Name);
		FEditableTextBoxStyle Style = Input->GetWidgetStyle();
		Style.SetBackgroundImageNormal(FSlateRoundedBoxBrush(FLinearColor::White, ControlCornerRadius));
		Style.SetBackgroundImageHovered(FSlateRoundedBoxBrush(FLinearColor::White, ControlCornerRadius));
		Style.SetBackgroundImageFocused(FSlateRoundedBoxBrush(FLinearColor::White, ControlCornerRadius,
			FLinearColor::FromSRGBColor(FColor(216, 185, 113)), FocusOutlineWidth));
		Style.SetBackgroundColor(FLinearColor::White);
		Style.SetForegroundColor(Ink);
		Style.SetFocusedForegroundColor(Ink);
		Style.SetPadding(InputPadding);
		FTextBlockStyle TextStyle = Style.TextStyle;
		TextStyle.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", InputFont));
		TextStyle.SetColorAndOpacity(Ink);
		Style.SetTextStyle(TextStyle);
		Input->SetWidgetStyle(Style);
		// Slate renders hint text with reduced opacity, producing light gray on white.
		Input->SetHintText(Hint);
		Input->SetIsPassword(bPassword);
		Input->SetClearKeyboardFocusOnCommit(false);
		Input->OnTextChanged.AddDynamic(this, &UFWLoginWidget::OnCredentialsChanged);
		USizeBox* InputSize = WidgetTree->ConstructWidget<USizeBox>();
		InputSize->SetHeightOverride(InputHeight);
		InputSize->SetContent(Input);
		AddRow(InputSize, bPassword ? PasswordBottomGap : InputGap);
		return Input;
	};
	UsernameInput = AddInput(TEXT("UsernameInput"), LOCTEXT("Username", "아이디"), false);
	PasswordInput = AddInput(TEXT("PasswordInput"), LOCTEXT("Password", "비밀번호"), true);
	UsernameInput->OnTextCommitted.AddDynamic(this, &UFWLoginWidget::OnUsernameCommitted);
	PasswordInput->OnTextCommitted.AddDynamic(this, &UFWLoginWidget::OnPasswordCommitted);

	auto MakeButton = [this, &MakeText, Cream, Muted, Ink](FName Name, FText Text, bool bPrimary, UTextBlock*& Label)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(FSlateRoundedBoxBrush(bPrimary ? Cream : FLinearColor::Transparent, ControlCornerRadius));
		Style.SetHovered(FSlateRoundedBoxBrush(bPrimary ? FLinearColor(1.f, 0.86f, 0.57f) : FLinearColor(1.f, 1.f, 1.f, 0.10f), ControlCornerRadius));
		Style.SetPressed(FSlateRoundedBoxBrush(bPrimary ? FLinearColor(0.64f, 0.48f, 0.24f) : FLinearColor(1.f, 1.f, 1.f, 0.16f), ControlCornerRadius));
		Style.SetDisabled(FSlateRoundedBoxBrush(bPrimary ? FLinearColor(0.3f, 0.27f, 0.21f, 0.7f) : FLinearColor::Transparent, ControlCornerRadius));
		Style.SetNormalPadding(FMargin(ButtonHorizontalPadding, bPrimary ? PrimaryVerticalPadding : SecondaryVerticalPadding));
		Style.SetPressedPadding(Style.NormalPadding);
		Button->SetStyle(Style);
		Label = MakeText(Text, bPrimary ? PrimaryFont : SecondaryFont, bPrimary ? Ink : Muted, bPrimary);
		Label->SetAutoWrapText(false);
		Button->SetContent(Label);
		return Button;
	};
	UTextBlock* Label = nullptr;
	LoginButton = MakeButton(TEXT("Login"), LOCTEXT("Login", "로그인"), true, Label);
	LoginButton->SetIsEnabled(false);
	LoginButton->OnClicked.AddDynamic(this, &UFWLoginWidget::OnLoginClicked);
	AddRow(LoginButton, ButtonBottomGap);

	UHorizontalBox* Links = WidgetTree->ConstructWidget<UHorizontalBox>();
	SignUpButton = MakeButton(TEXT("SignUp"), LOCTEXT("SignUp", "회원가입"), false, Label);
	SignUpButton->OnClicked.AddDynamic(this, &UFWLoginWidget::OnSignUpClicked);
	StartButton = MakeButton(TEXT("StartGame"), LOCTEXT("LocalPlay", "로컬 플레이"), false, Label);
	StartLabel = Label;
	StartButton->OnClicked.AddDynamic(this, &UFWLoginWidget::OnStartClicked);
	for (UButton* Button : { SignUpButton.Get(), StartButton.Get() })
	{
		Links->AddChildToHorizontalBox(Button)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}
	AddRow(Links, ButtonBottomGap);
	StatusLabel = MakeText(LOCTEXT("Offline", "계정 서비스 연결 준비 중"), StatusFont, Muted);
	USizeBox* StatusSize = WidgetTree->ConstructWidget<USizeBox>();
	StatusSize->SetHeightOverride(StatusHeight);
	StatusSize->SetContent(StatusLabel);
	AddRow(StatusSize, 0.f);

	// 하단 버튼과 도움말을 별도로 배치해 확대된 로그인 카드와 겹치지 않게 합니다.
	UHorizontalBox* UtilityButtons = WidgetTree->ConstructWidget<UHorizontalBox>();
	UCanvasPanelSlot* UtilitySlot = Canvas->AddChildToCanvas(UtilityButtons);
	UtilitySlot->SetAnchors(FAnchors(0.f, 1.f));
	UtilitySlot->SetAlignment(FVector2D(0.f, 1.f));
	UtilitySlot->SetPosition(FVector2D(LeftMargin, -UtilityBottomMargin));
	UtilitySlot->SetAutoSize(true);
	USizeBox* HelpWidth = WidgetTree->ConstructWidget<USizeBox>();
	HelpWidth->SetWidthOverride(HelpPanelWidth);
	UCanvasPanelSlot* HelpSlot = Canvas->AddChildToCanvas(HelpWidth);
	HelpSlot->SetAnchors(FAnchors(0.f, 1.f));
	HelpSlot->SetAlignment(FVector2D(0.f, 1.f));
	HelpSlot->SetPosition(FVector2D(LeftMargin + LoginLayout::CardWidth + HelpPanelGap, -UtilityBottomMargin));
	HelpSlot->SetAutoSize(true);
	ControlsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ControlsPanel"));
	ControlsPanel->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, ControlCornerRadius));
	ControlsPanel->SetBrushColor(FLinearColor::FromSRGBColor(FColor(19, 26, 24)));
	ControlsPanel->SetPadding(FMargin(HelpPadding));
	ControlsPanel->SetContent(MakeText(LOCTEXT("Help", "우클릭 · 이동 / 미니맵 이동\n휠 · 확대 / 축소   화면 가장자리 · 카메라 이동\nSpace · 따라가기   Y · 카메라 고정\nQ / W / E / R · 스킬"), HelpFont, Muted));
	ControlsPanel->SetVisibility(ESlateVisibility::Collapsed);
	HelpWidth->SetContent(ControlsPanel);
	ControlsButton = MakeButton(TEXT("Controls"), LOCTEXT("Controls", "조작 방법"), false, Label);
	ControlsLabel = Label;
	ControlsButton->OnClicked.AddDynamic(this, &UFWLoginWidget::OnControlsClicked);
	QuitButton = MakeButton(TEXT("QuitGame"), LOCTEXT("Quit", "게임 종료"), false, Label);
	QuitButton->OnClicked.AddDynamic(this, &UFWLoginWidget::OnQuitClicked);
	for (UButton* Button : { ControlsButton.Get(), QuitButton.Get() })
	{
		// 불투명(알파 1) 박스: 숲 배경과 패널 투명도 설정에 영향을 받지 않습니다.
		FButtonStyle Style = Button->GetStyle();
		Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor::FromSRGBColor(FColor(19, 26, 24)), ControlCornerRadius));
		Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor::FromSRGBColor(FColor(42, 55, 48)), ControlCornerRadius));
		Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor::FromSRGBColor(FColor(9, 15, 12)), ControlCornerRadius));
		Style.SetDisabled(Style.Normal);
		Button->SetStyle(Style);
		UTextBlock* ButtonLabel = CastChecked<UTextBlock>(Button->GetContent());
		ButtonLabel->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", UtilityFont));
		ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
		ButtonSize->SetWidthOverride(UtilityButtonWidth);
		ButtonSize->SetHeightOverride(UtilityButtonHeight);
		ButtonSize->SetContent(Button);
		UHorizontalBoxSlot* ButtonSlot = UtilityButtons->AddChildToHorizontalBox(ButtonSize);
		ButtonSlot->SetPadding(FMargin(0.f, 0.f, Button == ControlsButton ? UtilityButtonGap : 0.f, 0.f));
	}
}

TSharedPtr<SWidget> UFWLoginWidget::GetInitialFocusWidget() const
{
	return UsernameInput ? UsernameInput->GetCachedWidget() : GetCachedWidget();
}

void UFWLoginWidget::ShowStatus(const FText& Message)
{
	if (StatusLabel) StatusLabel->SetText(Message);
}

void UFWLoginWidget::OnCredentialsChanged(const FText& Text)
{
	if (LoginButton && UsernameInput && PasswordInput)
	{
		LoginButton->SetIsEnabled(!bStarting && !UsernameInput->GetText().ToString().TrimStartAndEnd().IsEmpty()
			&& !PasswordInput->GetText().IsEmpty());
	}
}

void UFWLoginWidget::OnUsernameCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter && PasswordInput) PasswordInput->SetKeyboardFocus();
}

void UFWLoginWidget::OnPasswordCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter) OnLoginClicked();
}

void UFWLoginWidget::OnLoginClicked()
{
	OnCredentialsChanged(FText::GetEmpty());
	if (bStarting || !LoginButton || !LoginButton->GetIsEnabled()) return;
	// The current C2S_Login protocol only accepts a username; it is not password authentication.
	// Do not transmit, log, persist, or treat these credentials as an authenticated session.
	PasswordInput->SetText(FText::GetEmpty());
	OnCredentialsChanged(FText::GetEmpty());
	ShowStatus(LOCTEXT("AuthUnavailable", "로그인 서버 연동 준비 중입니다.\n로컬 플레이로 먼저 둘러보세요."));
}

void UFWLoginWidget::OnSignUpClicked()
{
	ShowStatus(LOCTEXT("SignUpUnavailable", "회원가입은 계정 서버 연동 후 이용할 수 있습니다."));
}

void UFWLoginWidget::NativeDestruct()
{
	if (PasswordInput) PasswordInput->SetText(FText::GetEmpty());
	Super::NativeDestruct();
}

void UFWLoginWidget::SetStartingGame()
{
	bStarting = true;
	PasswordInput->SetText(FText::GetEmpty());
	UsernameInput->SetIsEnabled(false);
	PasswordInput->SetIsEnabled(false);
	LoginButton->SetIsEnabled(false);
	SignUpButton->SetIsEnabled(false);
	StartButton->SetIsEnabled(false);
	ControlsButton->SetIsEnabled(false);
	QuitButton->SetIsEnabled(false);
	StartLabel->SetText(LOCTEXT("Starting", "시작하는 중…"));
	ShowStatus(LOCTEXT("Loading", "전장을 불러오고 있습니다."));
}

void UFWLoginWidget::OnStartClicked()
{
	if (AFWLoginPlayerController* PC = GetOwningPlayer<AFWLoginPlayerController>()) PC->StartGame();
}

void UFWLoginWidget::OnControlsClicked()
{
	const bool bShow = ControlsPanel->GetVisibility() == ESlateVisibility::Collapsed;
	ControlsPanel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ControlsLabel->SetText(bShow ? LOCTEXT("CloseControls", "조작 방법 닫기") : LOCTEXT("Controls", "조작 방법"));
}

void UFWLoginWidget::OnQuitClicked()
{
	if (AFWLoginPlayerController* PC = GetOwningPlayer<AFWLoginPlayerController>()) PC->QuitGame();
}

#undef LOCTEXT_NAMESPACE
