#include "FWLobbyWidget.h"
#include "FWLoginPlayerController.h"
#include "FWLoginWindLayer.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "InputCoreTypes.h"

#define LOCTEXT_NAMESPACE "FWLobby"

namespace LobbyColors
{
	FLinearColor Cream() { return FLinearColor::FromSRGBColor(FColor(255, 241, 212)); }
	FLinearColor Ink() { return FLinearColor::FromSRGBColor(FColor(24, 57, 47)); }
	FLinearColor Muted() { return FLinearColor::FromSRGBColor(FColor(184, 195, 177)); }
}

UFWLobbyWidget::UFWLobbyWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BackgroundImagePath = TEXT("Art/UI/Lobby/Backgrounds/CloseIN_Lobby_Background_ForestEntrance_v1.png");
	BackgroundTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Login/T_LoginBackground_Forest_v3.T_LoginBackground_Forest_v3")));
	LogoTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Login/T_Logo_Mix_v7.T_Logo_Mix_v7")));
	MatchCompleteBackgroundImagePath = TEXT("Art/UI/Matchmaking/CloseIN_MatchComplete_Background_v1_1280.png");
	MatchCompleteTextImagePath = TEXT("Art/UI/Matchmaking/CloseIN_MatchComplete_TextOnly_v1_1280.png");
}

TSharedRef<SWidget> UFWLobbyWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLobby();
	}
	return Super::RebuildWidget();
}

UCanvasPanelSlot* UFWLobbyWidget::Place(UCanvasPanel* Canvas, UWidget* Widget, FVector2D Position, FVector2D Size)
{
	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(Widget);
	CanvasSlot->SetPosition(Position);
	CanvasSlot->SetSize(Size);
	return CanvasSlot;
}

UTexture2D* UFWLobbyWidget::LoadComponent(FName Name)
{
	if (const TObjectPtr<UTexture2D>* Found = ComponentTextures.Find(Name))
	{
		return Found->Get();
	}
	const FString Key = Name.ToString();
	const bool bTeamSkin = Key.StartsWith(TEXT("team-"));
	const FString Path = FPaths::Combine(FPaths::ProjectDir(),
		bTeamSkin ? TEXT("Art/UI/MenuTeamConcept/Components") : TEXT("Art/UI/Lobby/Components"),
		(bTeamSkin ? Key.RightChop(5) : Key) + TEXT(".png"));
	UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(Path);
	ComponentTextures.Add(Name, Texture);
	if (!Texture)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FWLobby] UI skin unavailable: %s"), *Path);
	}
	return Texture;
}

FSlateBrush UFWLobbyWidget::MakeSkin(FName Name)
{
	if (UTexture2D* Texture = LoadComponent(Name))
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		// Slate Box는 텍스처 픽셀 크기로 모서리를 계산하므로 게임용 PNG는 128x96이다.
		const bool bTeamSkin = Name.ToString().StartsWith(TEXT("team-"));
		Brush.ImageSize = bTeamSkin ? FVector2D(160.f, 120.f) : FVector2D(128.f, 96.f);
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = bTeamSkin ? FMargin(.2f, 32.f / 120.f, .2f, 32.f / 120.f) : FMargin(.1875f, .25f, .1875f, .25f);
		Brush.TintColor = FSlateColor(FLinearColor::White);
		return Brush;
	}
	// 파일 누락 시에도 버튼의 영역과 문자를 읽을 수 있는 대체 스타일.
	return FSlateRoundedBoxBrush(FLinearColor(.16f, .27f, .20f), 12.f);
}

void UFWLobbyWidget::StyleButton(UButton* Button, const FString& Family)
{
	FButtonStyle Style;
	Style.SetNormal(MakeSkin(FName(*(Family + TEXT("-default")))));
	Style.SetHovered(MakeSkin(FName(*(Family + TEXT("-hover")))));
	Style.SetPressed(MakeSkin(FName(*(Family + TEXT("-pressed")))));
	Style.SetDisabled(MakeSkin(TEXT("disabled")));
	Style.SetNormalPadding(FMargin(0.f, 0.f, 0.f, 5.f));
	Style.SetPressedPadding(Style.NormalPadding);
	Button->SetStyle(Style);
}

void UFWLobbyWidget::BuildLobby()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	Root->SetClipping(EWidgetClipping::ClipToBounds);
	WidgetTree->RootWidget = Root;
	auto Fill = [Root](UWidget* Widget)
	{
		UOverlaySlot* OverlaySlot = Root->AddChildToOverlay(Widget);
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	};
	UScaleBox* Cover = WidgetTree->ConstructWidget<UScaleBox>();
	Cover->SetStretch(EStretch::ScaleToFill);
	Cover->SetVisibility(ESlateVisibility::HitTestInvisible);
	Fill(Cover);
	UImage* Background = WidgetTree->ConstructWidget<UImage>();
	if (!BackgroundImagePath.IsEmpty())
	{
		LoadedBackgroundTexture = FImageUtils::ImportFileAsTexture2D(FPaths::Combine(FPaths::ProjectDir(), BackgroundImagePath));
	}
	const bool bUsingOriginalBackground = !LoadedBackgroundTexture;
	if (!LoadedBackgroundTexture) LoadedBackgroundTexture = BackgroundTexture.LoadSynchronous();
	if (LoadedBackgroundTexture)
	{
		Background->SetBrushFromTexture(LoadedBackgroundTexture, true);
	}
	else
	{
		Background->SetColorAndOpacity(FLinearColor(.02f, .04f, .025f));
	}
	Cover->SetContent(Background);
	// 기존 가지 좌표/팔레트는 로그인 원본 전용. 새 숲 입구에는 잘못 겹치지 않는다.
	if (bUsingOriginalBackground)
	{
		UFWLoginWindLayer* Wind = WidgetTree->ConstructWidget<UFWLoginWindLayer>();
		Wind->SetVisibility(ESlateVisibility::HitTestInvisible);
		Wind->SetClipping(EWidgetClipping::ClipToBounds);
		Fill(Wind);
	}

	UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
	Scale->SetStretch(EStretch::ScaleToFit);
	Fill(Scale);
	USizeBox* Design = WidgetTree->ConstructWidget<USizeBox>();
	Design->SetWidthOverride(1920.f);
	Design->SetHeightOverride(1080.f);
	Scale->SetContent(Design);
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	Design->SetContent(Canvas);
	LobbyContent = Canvas;
	auto Text = [this](FName Name, const FText& Value, int32 Size, FLinearColor Color, bool bBold)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Label->SetText(Value);
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
		Label->SetColorAndOpacity(Color);
		Label->SetJustification(ETextJustify::Center);
		Label->SetAutoWrapText(false);
		return Label;
	};
	auto Image = [this, Canvas](FName Name, UTexture2D* Texture, FVector2D Position, FVector2D Size)
	{
		UImage* Widget = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		Widget->SetBrushFromTexture(Texture);
		Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		Place(Canvas, Widget, Position, Size);
		return Widget;
	};
	Image(TEXT("HeaderShade"), LoadComponent(TEXT("header-shade")), FVector2D::ZeroVector, FVector2D(1920.f, 180.f));
	Image(TEXT("BottomShade"), LoadComponent(TEXT("bottom-shade")), FVector2D(0.f, 790.f), FVector2D(1920.f, 290.f));
	if (UTexture2D* Logo = LogoTexture.LoadSynchronous())
	{
		LobbyLogo = Image(TEXT("LobbyLogo"), Logo, FVector2D(64.f, 38.f), FVector2D(218.f, 109.f));
	}
	else
	{
		Place(Canvas, Text(TEXT("LobbyTitle"), LOCTEXT("Title", "Close IN"), 32, LobbyColors::Cream(), true), FVector2D(64.f, 62.f), FVector2D(218.f, 64.f));
	}
	BuildMainAndTeam(Canvas);
	BuildInvitation();
	RefreshScreen();

	// 원본 PNG 두 장을 같은 크기로 겹친다. 원본 파일이나 글자를 재생성하지 않는다.
	MatchCompleteBackground = FImageUtils::ImportFileAsTexture2D(FPaths::Combine(FPaths::ProjectDir(), MatchCompleteBackgroundImagePath));
	MatchCompleteText = FImageUtils::ImportFileAsTexture2D(FPaths::Combine(FPaths::ProjectDir(), MatchCompleteTextImagePath));
	UBorder* CompleteMask = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MatchCompleteMask"));
	CompleteMask->SetBrush(FSlateColorBrush(FLinearColor::White));
	CompleteMask->SetPadding(FMargin(0.f));
	FLinearColor Dim = FLinearColor::FromSRGBColor(FColor(0, 0, 0));
	Dim.A = .5f;
	CompleteMask->SetBrushColor(Dim);
	Fill(CompleteMask);
	UScaleBox* CompleteScale = WidgetTree->ConstructWidget<UScaleBox>();
	CompleteScale->SetStretch(EStretch::ScaleToFit);
	CompleteMask->SetContent(CompleteScale);
	USizeBox* CompleteDesign = WidgetTree->ConstructWidget<USizeBox>();
	CompleteDesign->SetWidthOverride(1920.f);
	CompleteDesign->SetHeightOverride(1080.f);
	CompleteScale->SetContent(CompleteDesign);
	UCanvasPanel* CompleteCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	CompleteDesign->SetContent(CompleteCanvas);
	UOverlay* CompleteBanner = WidgetTree->ConstructWidget<UOverlay>();
	UCanvasPanelSlot* BannerCanvasSlot = CompleteCanvas->AddChildToCanvas(CompleteBanner);
	BannerCanvasSlot->SetAnchors(FAnchors(.5f, .5f));
	BannerCanvasSlot->SetAlignment(FVector2D(.5f, .5f));
	BannerCanvasSlot->SetPosition(FVector2D::ZeroVector);
	BannerCanvasSlot->SetSize(FVector2D(1040.f, 1040.f * 457.f / 1280.f));
	auto AddBannerImage = [this, CompleteBanner](UTexture2D* Texture)
	{
		if (!Texture) return;
		UImage* Layer = WidgetTree->ConstructWidget<UImage>();
		Layer->SetBrushFromTexture(Texture);
		Layer->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* LayerOverlaySlot = CompleteBanner->AddChildToOverlay(Layer);
		LayerOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		LayerOverlaySlot->SetVerticalAlignment(VAlign_Fill);
	};
	AddBannerImage(MatchCompleteBackground);
	AddBannerImage(MatchCompleteText);
	if (!MatchCompleteText)
	{
		UTextBlock* Fallback = Text(TEXT("MatchCompleteFallback"), LOCTEXT("MatchComplete", "매칭 완료"), 64, LobbyColors::Cream(), true);
		UOverlaySlot* FallbackOverlaySlot = CompleteBanner->AddChildToOverlay(Fallback);
		FallbackOverlaySlot->SetHorizontalAlignment(HAlign_Center);
		FallbackOverlaySlot->SetVerticalAlignment(VAlign_Center);
	}
	CompleteMask->SetVisibility(ESlateVisibility::Collapsed);
	MatchCompleteOverlay = CompleteMask;
}

UTextBlock* UFWLobbyWidget::MakeText(FName Name, const FText& Value, int32 Size, FLinearColor Color, bool bBold)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Label->SetText(Value);
	Label->SetFont(FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size));
	Label->SetColorAndOpacity(Color);
	Label->SetJustification(ETextJustify::Center);
	Label->SetAutoWrapText(false);
	Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Label;
}

UImage* UFWLobbyWidget::MakeImage(UCanvasPanel* Canvas, FName Name, UTexture2D* Texture, FVector2D Position, FVector2D Size)
{
	UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
	Image->SetBrushFromTexture(Texture);
	Image->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Canvas, Image, Position, Size);
	return Image;
}

UButton* UFWLobbyWidget::MakeButton(UCanvasPanel* Canvas, FName Name, const FText& Value, FVector2D Position, FVector2D Size, const TCHAR* Family, int32 FontSize, UTextBlock*& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	StyleButton(Button, Family);
	Label = MakeText(FName(*(Name.ToString() + TEXT("Label"))), Value, FontSize,
		FCString::Strcmp(Family, TEXT("secondary")) == 0 ? LobbyColors::Ink() : LobbyColors::Cream(), FCString::Strcmp(Family, TEXT("primary")) == 0);
	Button->SetContent(Label);
	Place(Canvas, Button, Position, Size);
	return Button;
}

void UFWLobbyWidget::BuildMainAndTeam(UCanvasPanel* Canvas)
{
	// 모든 좌표/크기는 승인 HTML과 동일한 1920x1080 기준 UI 단위입니다.
	constexpr float CardWidth = 320.f; // 팀원 카드 가로 크기
	constexpr float CardHeight = 600.f; // 팀원 카드 세로 크기
	constexpr float CardGap = 24.f; // 카드 사이 간격
	const FVector2D ProfileSize(104.f, 114.f); // 중립 프로필 표시 크기
	UCanvasPanel* Main = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MainLobbyContent"));
	Place(Canvas, Main, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f));
	MainContent = Main;
	MainNickname = MakeText(TEXT("LobbyNickname"), FText::FromString(LocalNickname), 23, LobbyColors::Cream(), true);
	MainNickname->SetJustification(ETextJustify::Right);
	Place(Main, MainNickname, FVector2D(1571.f, 66.5f), FVector2D(120.f, 38.f));
	UBorder* Divider = WidgetTree->ConstructWidget<UBorder>();
	Divider->SetBrushColor(FLinearColor(1.f, .9f, .7f, .35f));
	Divider->SetVisibility(ESlateVisibility::HitTestInvisible);
	Place(Main, Divider, FVector2D(1717.f, 71.f), FVector2D(1.f, 22.f));
	UTextBlock* Label = nullptr;
	QuitButton = MakeButton(Main, TEXT("LobbyQuit"), LOCTEXT("Exit", "종료"), FVector2D(1744.f, 54.f), FVector2D(112.f, 56.f), TEXT("quiet"), 21, Label);
	QuitButton->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnQuitClicked);
	GameStartButton = MakeButton(Main, TEXT("LobbyGameStart"), LOCTEXT("GameStart", "게임 시작"), FVector2D(664.f, 890.f), FVector2D(400.f, 88.f), TEXT("primary"), 29, Label);
	GameStartButton->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnGameStartClicked);
	MakeImage(Main, TEXT("MainLeafLeft"), LoadComponent(TEXT("leaf-left")), FVector2D(690.f, 915.f), FVector2D(56.f, 32.f));
	MakeImage(Main, TEXT("MainLeafRight"), LoadComponent(TEXT("leaf-right")), FVector2D(982.f, 915.f), FVector2D(56.f, 32.f));
	CharacterButton = MakeButton(Main, TEXT("LobbyCharacter"), LOCTEXT("Character", "캐릭터"), FVector2D(1080.f, 898.f), FVector2D(176.f, 72.f), TEXT("secondary"), 23, Label);
	CharacterLabel = Label;
	CharacterButton->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnCharacterClicked);

	TeamContent = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TeamLobbyContent"));
	Place(Canvas, TeamContent, FVector2D::ZeroVector, FVector2D(1920.f, 1080.f));
	BackButton = MakeButton(TeamContent, TEXT("TeamLobbyBack"), LOCTEXT("Back", "뒤로"), FVector2D(64.f, 54.f), FVector2D(124.f, 56.f), TEXT("quiet"), 21, Label);
	BackButton->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnBackClicked);
	MakeImage(TeamContent, TEXT("TeamBackArrow"), LoadComponent(TEXT("team-arrow-back")), FVector2D(80.f, 73.f), FVector2D(18.f, 18.f));
	TeamGroup = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TeamPlayerGroup"));
	Place(TeamContent, TeamGroup, FVector2D(456.f, 208.f), FVector2D(1008.f, 800.f));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const float X = Index * (CardWidth + CardGap);
		UImage* Card = MakeImage(TeamGroup, FName(*FString::Printf(TEXT("PlayerCard_%d"), Index)), nullptr, FVector2D(X, 0.f), FVector2D(CardWidth, CardHeight));
		Card->SetBrush(MakeSkin(Index == 0 ? TEXT("team-card-filled") : TEXT("team-card-empty")));
		if (Index == 0)
		{
			LeaderCard = Card;
			MakeImage(TeamGroup, TEXT("LeaderMark"), LoadComponent(TEXT("team-leader-mark")), FVector2D(X + 24.f, 25.f), FVector2D(12.f, 12.f));
			Place(TeamGroup, MakeText(TEXT("LeaderLabel"), LOCTEXT("Leader", "팀장"), 15, LobbyColors::Ink(), true), FVector2D(X + 42.f, 21.f), FVector2D(44.f, 24.f));
			MakeImage(TeamGroup, TEXT("LeaderProfile"), LoadComponent(TEXT("team-profile-neutral")), FVector2D(X + 108.f, 200.f), ProfileSize);
			LeaderNickname = MakeText(TEXT("TeamLeaderNickname"), FText::FromString(LocalNickname), 26, LobbyColors::Ink(), true);
			LeaderNickname->SetClipping(EWidgetClipping::ClipToBounds);
			Place(TeamGroup, LeaderNickname, FVector2D(X + 24.f, 344.f), FVector2D(272.f, 42.f));
		}
		else
		{
			UButton* Invite = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), FName(*FString::Printf(TEXT("TeamInvite_%d"), Index)));
			FButtonStyle Style;
			Style.SetNormal(FSlateColorBrush(FLinearColor::Transparent));
			Style.SetHovered(FSlateColorBrush(FLinearColor(1.f, .85f, .6f, .05f)));
			Style.SetPressed(Style.Normal); Style.SetDisabled(Style.Normal);
			Style.SetNormalPadding(FMargin(0)); Style.SetPressedPadding(FMargin(0));
			Invite->SetStyle(Style);
			UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>();
			Invite->SetContent(Content);
			UButtonSlot* InviteContentSlot = CastChecked<UButtonSlot>(Content->Slot);
			InviteContentSlot->SetHorizontalAlignment(HAlign_Fill);
			InviteContentSlot->SetVerticalAlignment(VAlign_Fill);
			MakeImage(Content, FName(*FString::Printf(TEXT("InvitePlus_%d"), Index)), LoadComponent(TEXT("team-invite-plus")), FVector2D(131.f, 244.f), FVector2D(58.f, 58.f));
			Place(Content, MakeText(FName(*FString::Printf(TEXT("InviteText_%d"), Index)), LOCTEXT("Invite", "초대"), 26, LobbyColors::Cream()), FVector2D(24.f, 324.f), FVector2D(272.f, 42.f));
			Place(TeamGroup, Invite, FVector2D(X, 0.f), FVector2D(CardWidth, CardHeight));
			Invite->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnInviteClicked);
			InviteButtons.Add(Invite);
		}
	}
	UBorder* Status = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MatchSearchStatus"));
	Status->SetBrush(MakeSkin(TEXT("quiet-default")));
	Status->SetPadding(FMargin(0));
	UCanvasPanel* StatusContent = WidgetTree->ConstructWidget<UCanvasPanel>();
	Status->SetContent(StatusContent);
	Place(StatusContent, MakeText(TEXT("SearchText"), LOCTEXT("Searching", "매칭 중…"), 21, LobbyColors::Cream()), FVector2D(20.f, 9.f), FVector2D(140.f, 35.f));
	ElapsedLabel = MakeText(TEXT("MatchElapsed"), FText::FromString(TEXT("00:00")), 21, LobbyColors::Cream());
	Place(StatusContent, ElapsedLabel, FVector2D(182.f, 9.f), FVector2D(74.f, 35.f));
	Place(TeamGroup, Status, FVector2D(366.f, 632.f), FVector2D(276.f, 54.f));
	SearchStatus = Status;
	MatchButton = MakeButton(TeamGroup, TEXT("LobbyMatch"), LOCTEXT("Start", "매칭 시작"), FVector2D(304.f, 632.f), FVector2D(400.f, 84.f), TEXT("primary"), 29, Label);
	MatchLabel = Label;
	MatchButton->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnMatchClicked);
	ButtonOrnaments.Add(MakeImage(TeamGroup, TEXT("MatchLeafLeft"), LoadComponent(TEXT("leaf-left")), FVector2D(330.f, 655.f), FVector2D(56.f, 32.f)));
	ButtonOrnaments.Add(MakeImage(TeamGroup, TEXT("MatchLeafRight"), LoadComponent(TEXT("leaf-right")), FVector2D(622.f, 655.f), FVector2D(56.f, 32.f)));
	NoteLabel = MakeText(TEXT("LobbyNote"), FText::GetEmpty(), 16, LobbyColors::Cream());
	NoteLabel->SetShadowOffset(FVector2D(0.f, 1.f));
	NoteLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, .7f));
	Place(Canvas, NoteLabel, FVector2D(384.f, 1006.f), FVector2D(1152.f, 36.f));
}

void UFWLobbyWidget::BuildInvitation()
{
	UBorder* Mask = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InvitationMask"));
	Mask->SetBrush(FSlateColorBrush(FLinearColor::White));
	FLinearColor Dim = FLinearColor::FromSRGBColor(FColor(0,0,0)); Dim.A = .5f; // 배경 어둡기
	Mask->SetBrushColor(Dim); Mask->SetPadding(FMargin(0));
	UOverlaySlot* MaskOverlaySlot = CastChecked<UOverlay>(WidgetTree->RootWidget)->AddChildToOverlay(Mask);
	MaskOverlaySlot->SetHorizontalAlignment(HAlign_Fill); MaskOverlaySlot->SetVerticalAlignment(VAlign_Fill);
	UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>(); Scale->SetStretch(EStretch::ScaleToFit); Mask->SetContent(Scale);
	USizeBox* Design = WidgetTree->ConstructWidget<USizeBox>(); Design->SetWidthOverride(1920.f); Design->SetHeightOverride(1080.f); Scale->SetContent(Design);
	UCanvasPanel* ModalCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(); Design->SetContent(ModalCanvas);
	UBorder* Surface = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InvitationDialog"));
	Surface->SetBrush(MakeSkin(TEXT("team-dialog-surface"))); Surface->SetPadding(FMargin(0));
	UCanvasPanelSlot* DialogCanvasSlot = Place(ModalCanvas, Surface, FVector2D::ZeroVector, FVector2D(592.f, 368.f)); // 초대창 크기
	DialogCanvasSlot->SetAnchors(FAnchors(.5f,.5f)); DialogCanvasSlot->SetAlignment(FVector2D(.5f,.5f));
	UCanvasPanel* Content = WidgetTree->ConstructWidget<UCanvasPanel>(); Surface->SetContent(Content);
	MakeImage(Content, TEXT("InvitationLeaf"), LoadComponent(TEXT("leaf-left")), FVector2D(40.f, 40.f), FVector2D(45.f, 27.f));
	Place(Content, MakeText(TEXT("InviteHeading"), LOCTEXT("InviteHeading", "팀원 초대"), 29, LobbyColors::Ink(), true), FVector2D(98.f, 34.f), FVector2D(180.f, 44.f));
	Place(Content, MakeText(TEXT("InviteNicknameLabel"), LOCTEXT("Nickname", "닉네임"), 19, LobbyColors::Ink(), true), FVector2D(40.f, 104.f), FVector2D(80.f, 30.f));
	InviteInput = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), TEXT("InviteNicknameInput"));
	FEditableTextBoxStyle Style = InviteInput->GetWidgetStyle();
	const FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(255,249,235));
	Style.SetBackgroundImageNormal(FSlateRoundedBoxBrush(Paper,10.f)); Style.SetBackgroundImageHovered(Style.BackgroundImageNormal); Style.SetBackgroundImageFocused(Style.BackgroundImageNormal);
	Style.SetForegroundColor(LobbyColors::Ink()); Style.SetFocusedForegroundColor(LobbyColors::Ink());
	FTextBlockStyle TextStyle = Style.TextStyle; TextStyle.SetFont(FCoreStyle::GetDefaultFontStyle("Regular",23)); TextStyle.SetColorAndOpacity(LobbyColors::Ink()); Style.SetTextStyle(TextStyle);
	Style.SetPadding(FMargin(18.f,12.f));
	InviteInput->SetWidgetStyle(Style); InviteInput->SetHintText(LOCTEXT("InviteHint", "닉네임 입력")); InviteInput->SetClearKeyboardFocusOnCommit(false);
	Place(Content, InviteInput, FVector2D(40.f, 144.f), FVector2D(512.f, 62.f));
	InviteInput->OnTextChanged.AddDynamic(this, &UFWLobbyWidget::OnInviteTextChanged);
	InviteInput->OnTextCommitted.AddDynamic(this, &UFWLobbyWidget::OnInviteCommitted);
	InviteStatus = MakeText(TEXT("InviteStatus"), FText::GetEmpty(), 15, LobbyColors::Ink());
	Place(Content, InviteStatus, FVector2D(40.f, 209.f), FVector2D(512.f, 28.f));
	UTextBlock* Label = nullptr;
	UButton* Cancel = MakeButton(Content, TEXT("InviteCancel"), LOCTEXT("CancelInvite", "취소"), FVector2D(242.f, 244.f), FVector2D(148.f, 64.f), TEXT("secondary"), 22, Label);
	Cancel->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnInviteCancel);
	InviteSubmitButton = MakeButton(Content, TEXT("InviteSubmit"), LOCTEXT("Invite", "초대"), FVector2D(404.f, 244.f), FVector2D(148.f, 64.f), TEXT("primary"), 22, Label);
	InviteSubmitButton->OnClicked.AddDynamic(this, &UFWLobbyWidget::OnInviteSubmit);
	InviteSubmitButton->SetIsEnabled(false);
	Mask->SetVisibility(ESlateVisibility::Collapsed);
	InvitationOverlay = Mask;
}

void UFWLobbyWidget::ShowMatchComplete()
{
	CloseInvitation();
	bShowingMatchComplete = true;
	RefreshScreen();
	if (SearchStatus) SearchStatus->SetVisibility(ESlateVisibility::Collapsed);
	if (NoteLabel) NoteLabel->SetText(FText::GetEmpty());
	if (MatchCompleteOverlay) MatchCompleteOverlay->SetVisibility(ESlateVisibility::Visible);
}

void UFWLobbyWidget::HideMatchComplete()
{
	bShowingMatchComplete = false;
	if (MatchCompleteOverlay) MatchCompleteOverlay->SetVisibility(ESlateVisibility::Collapsed);
	if (LobbyContent) LobbyContent->SetIsEnabled(true);
	SetSearching(false);
}

TSharedPtr<SWidget> UFWLobbyWidget::GetInitialFocusWidget() const
{
	if (bInviteOpen && InviteInput) return InviteInput->GetCachedWidget();
	UButton* Target = bTeamLobby ? MatchButton.Get() : GameStartButton.Get();
	return Target ? Target->GetCachedWidget() : TSharedPtr<SWidget>();
}

void UFWLobbyWidget::RefreshScreen()
{
	const bool bBlocked = bTravelling || bShowingMatchComplete || bInviteOpen;
	if (LobbyContent)
	{
		// 모달 뒤 배경/카드를 회색으로 만들지 않고 입력만 차단합니다.
		LobbyContent->SetVisibility(bBlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::SelfHitTestInvisible);
	}
	if (MainContent) MainContent->SetVisibility(bTeamLobby ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (TeamContent) TeamContent->SetVisibility(bTeamLobby ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (LobbyLogo)
	{
		if (UCanvasPanelSlot* LogoCanvasSlot = Cast<UCanvasPanelSlot>(LobbyLogo->Slot))
		{
			LogoCanvasSlot->SetPosition(bTeamLobby ? FVector2D(224.f,34.f) : FVector2D(64.f,38.f));
			LogoCanvasSlot->SetSize(bTeamLobby ? FVector2D(188.f,94.f) : FVector2D(218.f,109.f));
		}
	}
	if (TeamGroup)
	{
		CastChecked<UCanvasPanelSlot>(TeamGroup->Slot)->SetPosition(FVector2D(456.f,bSearching ? 148.f : 208.f));
	}
	if (GameStartButton) GameStartButton->SetIsEnabled(!bBlocked);
	if (CharacterButton) CharacterButton->SetIsEnabled(!bBlocked);
	if (BackButton) BackButton->SetIsEnabled(!bSearching && !bBlocked);
	if (QuitButton) QuitButton->SetIsEnabled(!bBlocked);
	for (UButton* Invite : InviteButtons) Invite->SetIsEnabled(!bSearching && !bBlocked);
	if (LeaderCard) LeaderCard->SetBrush(MakeSkin(bSearching ? TEXT("team-card-searching") : TEXT("team-card-filled")));
	if (SearchStatus) SearchStatus->SetVisibility(bSearching && !bShowingMatchComplete ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (MatchButton)
	{
		StyleButton(MatchButton,bSearching ? TEXT("secondary") : TEXT("primary"));
		MatchButton->SetIsEnabled(bTeamLobby && !bBlocked);
		CastChecked<UCanvasPanelSlot>(MatchButton->Slot)->SetPosition(FVector2D(304.f,bSearching ? 700.f : 632.f));
	}
	if (MatchLabel)
	{
		MatchLabel->SetText(bSearching ? LOCTEXT("Cancel","매칭 취소") : LOCTEXT("Start","매칭 시작"));
		MatchLabel->SetColorAndOpacity(bSearching ? LobbyColors::Ink() : LobbyColors::Cream());
	}
	for (UWidget* Ornament : ButtonOrnaments) Ornament->SetVisibility(bSearching ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}

void UFWLobbyWidget::SetSearching(bool bInSearching, double ElapsedSeconds)
{
	if (bInSearching && (!bTeamLobby || bTravelling || bShowingMatchComplete)) return;
	const bool bChanged = bSearching != bInSearching;
	bSearching = bInSearching;
	if (ElapsedLabel)
	{
		const int32 Seconds = FMath::Max(0,FMath::FloorToInt(ElapsedSeconds));
		ElapsedLabel->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"),Seconds/60,Seconds%60)));
	}
	if (bChanged || !bSearching)
	{
		if (bSearching) CloseInvitation();
		ShowStatus(bSearching ? LOCTEXT("RosterLocked","매칭 중에는 초대와 팀원 변경이 잠깁니다") : FText::GetEmpty());
		RefreshScreen();
	}
}

void UFWLobbyWidget::SetLocalNickname(const FString& Nickname)
{
	LocalNickname = Nickname.TrimStartAndEnd().Left(20);
	if (LocalNickname.IsEmpty()) LocalNickname = TEXT("닉네임");
	if (MainNickname) MainNickname->SetText(FText::FromString(LocalNickname));
	if (LeaderNickname) LeaderNickname->SetText(FText::FromString(LocalNickname));
}

void UFWLobbyWidget::ShowStatus(const FText& Message)
{
	if (NoteLabel) NoteLabel->SetText(Message);
}

void UFWLobbyWidget::SetTravelling()
{
	bTravelling = true;
	RefreshScreen();
}

void UFWLobbyWidget::OnGameStartClicked()
{
	if (bTeamLobby || bSearching || bTravelling || bInviteOpen || bShowingMatchComplete) return;
	bTeamLobby = true;
	ShowStatus(FText::GetEmpty());
	RefreshScreen();
	if (MatchButton) MatchButton->SetKeyboardFocus();
}

void UFWLobbyWidget::OnBackClicked()
{
	if (!bTeamLobby || bSearching || bTravelling || bInviteOpen || bShowingMatchComplete) return;
	bTeamLobby = false;
	ShowStatus(FText::GetEmpty());
	RefreshScreen();
	if (GameStartButton) GameStartButton->SetKeyboardFocus();
}

void UFWLobbyWidget::OnInviteClicked()
{
	if (!bTeamLobby || bSearching || bTravelling || bInviteOpen || bShowingMatchComplete) return;
	bInviteOpen = true;
	InviteInput->SetText(FText::GetEmpty());
	InviteStatus->SetText(FText::GetEmpty());
	InviteSubmitButton->SetIsEnabled(false);
	InvitationOverlay->SetVisibility(ESlateVisibility::Visible);
	RefreshScreen();
	InviteInput->SetKeyboardFocus();
}

void UFWLobbyWidget::CloseInvitation()
{
	if (!bInviteOpen) return;
	bInviteOpen = false;
	if (InvitationOverlay) InvitationOverlay->SetVisibility(ESlateVisibility::Collapsed);
	if (InviteInput) InviteInput->SetText(FText::GetEmpty());
	RefreshScreen();
	if (MatchButton && !bSearching && !bTravelling && !bShowingMatchComplete) MatchButton->SetKeyboardFocus();
}

void UFWLobbyWidget::OnInviteCancel() { CloseInvitation(); }

void UFWLobbyWidget::OnInviteTextChanged(const FText& Text)
{
	if (!InviteSubmitButton) return;
	if (Text.ToString().Len() > 20)
	{
		InviteInput->SetText(FText::FromString(Text.ToString().Left(20)));
		return;
	}
	InviteSubmitButton->SetIsEnabled(bInviteOpen && !Text.ToString().TrimStartAndEnd().IsEmpty());
	if (InviteStatus) InviteStatus->SetText(FText::GetEmpty());
}

void UFWLobbyWidget::OnInviteCommitted(const FText& Text, ETextCommit::Type Method)
{
	if (Method == ETextCommit::OnEnter) OnInviteSubmit();
}

void UFWLobbyWidget::OnInviteSubmit()
{
	if (!bInviteOpen || bSearching || bTravelling || bShowingMatchComplete) return;
	const FString Nickname = InviteInput->GetText().ToString().TrimStartAndEnd();
	if (Nickname.IsEmpty()) return;
	if (Nickname.Equals(LocalNickname,ESearchCase::IgnoreCase))
	{
		InviteStatus->SetText(LOCTEXT("CannotInviteSelf","자신에게는 초대할 수 없습니다."));
		return;
	}
	CloseInvitation();
	// 실제 초대 응답 없이 가짜 팀원을 카드에 추가하지 않습니다.
	ShowStatus(LOCTEXT("InviteUnavailable","초대 서버 연동 준비 중입니다. 아직 초대 요청은 전송되지 않았습니다."));
}

FReply UFWLobbyWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Escape)
	{
		if (bInviteOpen) { CloseInvitation(); return FReply::Handled(); }
		if (bTeamLobby && !bSearching && !bShowingMatchComplete && !bTravelling)
		{
			OnBackClicked();
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewKeyDown(Geometry,Event);
}

void UFWLobbyWidget::OnMatchClicked()
{
	if (!bTeamLobby || bInviteOpen || bTravelling || bShowingMatchComplete) return;
	if (AFWLoginPlayerController* PC = GetOwningPlayer<AFWLoginPlayerController>())
	{
		if (bSearching) PC->CancelPrototypeMatch();
		else PC->BeginPrototypeMatch();
	}
}

void UFWLobbyWidget::OnCharacterClicked()
{
	if (!bSearching && !bTravelling && !bShowingMatchComplete) ShowStatus(LOCTEXT("CharacterPending","캐릭터 메뉴는 준비 중입니다."));
}

void UFWLobbyWidget::OnQuitClicked()
{
	if (AFWLoginPlayerController* PC = GetOwningPlayer<AFWLoginPlayerController>()) PC->QuitGame();
}

#undef LOCTEXT_NAMESPACE
