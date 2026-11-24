#include "Frontend/CharacterSelect/FWTeammateSelectSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "FWTeammateSelectSlot"

void UFWTeammateSelectSlot::ApplyView(const FFWTeamSelectPlayerView& View, UTexture2D* PortraitTexture)
{
	if (!View.bOccupied)
	{
		SetEmpty();
		return;
	}

	// 본인 강조(닉네임 색)와 확정 상태(아이콘/문구)는 서로 따로 갱신
	if (Txt_Nickname)
	{
		Txt_Nickname->SetText(View.Nickname.IsEmpty() ? LOCTEXT("UnknownPlayer", "플레이어") : FText::FromString(View.Nickname));
		Txt_Nickname->SetColorAndOpacity(FSlateColor(View.bIsLocalPlayer ? LocalNameColor : OtherNameColor));
	}

	SetPortrait(PortraitTexture);

	if (Img_Status)
	{
		UTexture2D* StatusTexture = View.bConfirmed ? StatusReadyTexture.Get() : StatusSelectingTexture.Get();
		if (StatusTexture)
		{
			Img_Status->SetBrushFromTexture(StatusTexture, false);
			Img_Status->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Img_Status->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Txt_Status)
	{
		Txt_Status->SetText(View.bConfirmed ? LOCTEXT("Ready", "확정") : LOCTEXT("Selecting", "선택 중"));
		Txt_Status->SetColorAndOpacity(FSlateColor(View.bConfirmed ? ReadyStatusColor : SelectingStatusColor));
		Txt_Status->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UFWTeammateSelectSlot::SetEmpty()
{
	if (Txt_Nickname)
	{
		Txt_Nickname->SetText(LOCTEXT("EmptySlot", "빈 슬롯"));
		Txt_Nickname->SetColorAndOpacity(FSlateColor(EmptyNameColor));
	}
	if (Img_Portrait)
	{
		Img_Portrait->SetVisibility(ESlateVisibility::Hidden);
	}
	if (Img_Status)
	{
		Img_Status->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (Txt_Status)
	{
		Txt_Status->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFWTeammateSelectSlot::SetPortrait(UTexture2D* Texture)
{
	if (!Img_Portrait)
	{
		return;
	}
	if (!Texture)
	{
		Img_Portrait->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	Img_Portrait->SetBrushFromTexture(Texture, false);
	Img_Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);
}

#undef LOCTEXT_NAMESPACE