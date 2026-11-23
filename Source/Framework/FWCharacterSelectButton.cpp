#include "FWCharacterSelectButton.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UFWCharacterSelectButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_Select)
	{
		Btn_Select->OnClicked.AddDynamic(this, &UFWCharacterSelectButton::HandleSelectClicked);
	}
	RefreshVisualState();
}

void UFWCharacterSelectButton::SetCharacterData(FName InCharacterId, const FFWCharacterSelectData& Data)
{
	CharacterId = InCharacterId;
	bSelectable = Data.bSelectable;

	if (Img_Portrait)
	{
		if (Data.Portrait)
		{
			// Match Size는 끄고 디자이너에서 정한 칸 크기를 그대로 사용
			Img_Portrait->SetBrushFromTexture(Data.Portrait, false);
			Img_Portrait->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Img_Portrait->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (Txt_CharacterName)
	{
		Txt_CharacterName->SetText(Data.DisplayName);
	}

	RefreshVisualState();
}

void UFWCharacterSelectButton::SetSelected(bool bIsSelected)
{
	bSelected = bIsSelected;
	RefreshVisualState();
}

void UFWCharacterSelectButton::SetSelectionLocked(bool bIsLocked)
{
	bSelectionLocked = bIsLocked;
	RefreshVisualState();
}

void UFWCharacterSelectButton::HandleSelectClicked()
{
	if (bSelectionLocked || CharacterId.IsNone())
	{
		return;
	}
	OnCharacterClicked.Broadcast(CharacterId);
}

void UFWCharacterSelectButton::RefreshVisualState()
{
	// 강조 이미지는 클릭을 가로채지 않도록 Not Hit-Testable로만 표시
	if (Img_SelectedGold)
	{
		Img_SelectedGold->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// SetIsEnabled(false)는 초상화까지 회색으로 그리므로, 잠금은 히트 테스트만 끄는 방식으로 처리
	if (Btn_Select)
	{
		Btn_Select->SetVisibility(bSelectionLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible);
	}

	const bool bDimmed = !bSelectable || (bSelectionLocked && !bSelected);
	if (Img_Portrait)
	{
		Img_Portrait->SetColorAndOpacity(bDimmed ? DimmedTint : FLinearColor::White);
	}
}