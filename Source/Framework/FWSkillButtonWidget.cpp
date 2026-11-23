#include "FWSkillButtonWidget.h"
#include "FWSkillTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UFWSkillButtonWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!Img_Icon)
	{
		return;
	}

	// 툴팁은 히트 테스트가 되는 위젯에서만 뜸
	Img_Icon->SetVisibility(ESlateVisibility::Visible);

	if (TooltipWidgetClass)
	{
		SkillTooltip = CreateWidget<UFWSkillTooltipWidget>(this, TooltipWidgetClass);
		if (SkillTooltip)
		{
			Img_Icon->SetToolTip(SkillTooltip);
		}
	}
}

void UFWSkillButtonWidget::SetSkillData(const FFWCharacterSkillData& Skill, const FText& FallbackKey, bool bHasData)
{
	const FText KeyText = (bHasData && !Skill.Key.IsEmpty()) ? Skill.Key : FallbackKey;

	if (Txt_Key)
	{
		Txt_Key->SetText(KeyText);
	}

	if (Img_Icon)
	{
		if (bHasData && Skill.Icon)
		{
			Img_Icon->SetBrushFromTexture(Skill.Icon, false);
			Img_Icon->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			// 이전 캐릭터의 아이콘이 남지 않도록 비우고 흐리게 표시
			Img_Icon->SetBrushFromTexture(nullptr, false);
			Img_Icon->SetColorAndOpacity(PlaceholderTint);
		}
	}

	if (SkillTooltip)
	{
		SkillTooltip->SetSkillData(Skill, KeyText, bHasData);
	}
}