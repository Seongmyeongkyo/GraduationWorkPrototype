#include "FWMainMenuWidget.h"
#include "FWMainMenuPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

#define LOCTEXT_NAMESPACE "FWMainMenu"

TSharedRef<SWidget> UFWMainMenuWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildMenu();
	}
	return Super::RebuildWidget();
}

void UFWMainMenuWidget::BuildMenu()
{
	const FLinearColor Muted(0.45f, 0.45f, 0.48f);
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

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlackBackground"));
	Background->SetBrushColor(FLinearColor::Black);
	Background->SetPadding(FMargin(32.f));
	WidgetTree->RootWidget = Background;

	// A fixed design width, scaled down for small windows. The background always fills the viewport.
	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>();
	Background->SetContent(Overlay);
	UScaleBox* ScaleBox = WidgetTree->ConstructWidget<UScaleBox>();
	ScaleBox->SetStretch(EStretch::ScaleToFit);
	ScaleBox->SetStretchDirection(EStretchDirection::DownOnly);
	UOverlaySlot* CenterSlot = Overlay->AddChildToOverlay(ScaleBox);
	CenterSlot->SetHorizontalAlignment(HAlign_Fill);
	CenterSlot->SetVerticalAlignment(VAlign_Fill);

	USizeBox* WidthBox = WidgetTree->ConstructWidget<USizeBox>();
	WidthBox->SetWidthOverride(440.f);
	ScaleBox->SetContent(WidthBox);
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	WidthBox->SetContent(Column);

	auto AddRow = [Column](UWidget* Widget, float BottomPadding)
	{
		UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(Widget);
		Slot->SetPadding(FMargin(0.f, 0.f, 0.f, BottomPadding));
		Slot->SetHorizontalAlignment(HAlign_Fill);
	};

	AddRow(MakeText(LOCTEXT("Eyebrow", "GRADUATION PROJECT"), 11, Muted), 12.f);
	AddRow(MakeText(LOCTEXT("Title", "FRAMEWORK"), 42, FLinearColor::White, true), 10.f);

	auto AddButton = [this, &MakeText, &AddRow](const FName& Name, const FText& Text, bool bPrimary, UTextBlock*& OutLabel)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		FButtonStyle Style = Button->GetStyle();
		auto FlatBrush = [](const FLinearColor& Color)
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.TintColor = FSlateColor(Color);
			return Brush;
		};
		Style.SetNormal(FlatBrush(bPrimary ? FLinearColor(0.88f, 0.88f, 0.88f) : FLinearColor(0.035f, 0.035f, 0.035f)));
		Style.SetHovered(FlatBrush(bPrimary ? FLinearColor::White : FLinearColor(0.09f, 0.09f, 0.09f)));
		Style.SetPressed(FlatBrush(bPrimary ? FLinearColor(0.65f, 0.65f, 0.65f) : FLinearColor(0.13f, 0.13f, 0.13f)));
		Style.SetNormalPadding(FMargin(20.f, 16.f));
		Style.SetPressedPadding(FMargin(20.f, 16.f));
		Button->SetStyle(Style);
		OutLabel = MakeText(Text, 17, bPrimary ? FLinearColor::Black : FLinearColor(0.75f, 0.75f, 0.78f), bPrimary);
		OutLabel->SetAutoWrapText(false);
		Button->SetContent(OutLabel);
		AddRow(Button, 10.f);
		return Button;
	};

	UTextBlock* Label = nullptr;
	StartButton = AddButton(TEXT("StartGame"), LOCTEXT("Start", "게임 시작"), true, Label);
	StartLabel = Label;
	StartButton->OnClicked.AddDynamic(this, &UFWMainMenuWidget::OnStartClicked);
	ControlsButton = AddButton(TEXT("Controls"), LOCTEXT("Controls", "조작 방법"), false, Label);
	ControlsLabel = Label;
	ControlsButton->OnClicked.AddDynamic(this, &UFWMainMenuWidget::OnControlsClicked);
	QuitButton = AddButton(TEXT("QuitGame"), LOCTEXT("Quit", "게임 종료"), false, Label);
	QuitButton->OnClicked.AddDynamic(this, &UFWMainMenuWidget::OnQuitClicked);

	ControlsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ControlsPanel"));
	ControlsPanel->SetBrushColor(FLinearColor(0.025f, 0.025f, 0.025f));
	ControlsPanel->SetPadding(FMargin(20.f));
	UTextBlock* Help = MakeText(LOCTEXT("Help", "우클릭  ·  이동 / 미니맵 이동\n마우스 휠  ·  화면 확대 / 축소\n화면 가장자리  ·  카메라 이동\nSpace 누르기  ·  캐릭터 따라가기\nY  ·  카메라 고정 전환\nQ / W / E / R  ·  스킬 입력 테스트"), 14, FLinearColor(0.65f, 0.65f, 0.68f));
	Help->SetJustification(ETextJustify::Left);
	ControlsPanel->SetContent(Help);
	ControlsPanel->SetVisibility(ESlateVisibility::Collapsed);
	AddRow(ControlsPanel, 14.f);

	StatusLabel = MakeText(LOCTEXT("Offline", "로컬 플레이 · 프로토타입"), 12, Muted);
	AddRow(StatusLabel, 0.f);
}

TSharedPtr<SWidget> UFWMainMenuWidget::GetInitialFocusWidget() const
{
	return StartButton ? StartButton->GetCachedWidget() : GetCachedWidget();
}

void UFWMainMenuWidget::ShowStatus(const FText& Message)
{
	if (StatusLabel)
	{
		StatusLabel->SetText(Message);
	}
}

void UFWMainMenuWidget::SetStartingGame()
{
	StartButton->SetIsEnabled(false);
	ControlsButton->SetIsEnabled(false);
	QuitButton->SetIsEnabled(false);
	StartLabel->SetText(LOCTEXT("Starting", "시작하는 중…"));
	ShowStatus(LOCTEXT("Loading", "전장을 불러오고 있습니다."));
}

void UFWMainMenuWidget::OnStartClicked()
{
	if (AFWMainMenuPlayerController* PC = GetOwningPlayer<AFWMainMenuPlayerController>())
	{
		PC->StartGame();
	}
}

void UFWMainMenuWidget::OnControlsClicked()
{
	const bool bShow = ControlsPanel->GetVisibility() == ESlateVisibility::Collapsed;
	ControlsPanel->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	ControlsLabel->SetText(bShow ? LOCTEXT("CloseControls", "조작 방법 닫기") : LOCTEXT("Controls", "조작 방법"));
}

void UFWMainMenuWidget::OnQuitClicked()
{
	if (AFWMainMenuPlayerController* PC = GetOwningPlayer<AFWMainMenuPlayerController>())
	{
		PC->QuitGame();
	}
}

#undef LOCTEXT_NAMESPACE
