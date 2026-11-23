#include "FWSkillTooltipWidget.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "FWSkillTooltip"

void UFWSkillTooltipWidget::SetSkillData(const FFWCharacterSkillData& Skill, const FText& KeyText, bool bHasData)
{
	if (!bHasData)
	{
		if (Txt_SkillName)
		{
			Txt_SkillName->SetText(FText::Format(LOCTEXT("PendingName", "{0} · 준비 중"), KeyText));
		}
		if (Txt_Description)
		{
			Txt_Description->SetText(LOCTEXT("PendingDescription", "스킬 정보가 아직 준비되지 않았습니다."));
		}
		if (Txt_Cooldown)
		{
			Txt_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (Txt_SkillName)
	{
		Txt_SkillName->SetText(FText::Format(LOCTEXT("SkillName", "{0} · {1}"), KeyText, Skill.DisplayName));
	}
	if (Txt_Description)
	{
		Txt_Description->SetText(Skill.Description.IsEmpty() ? LOCTEXT("EmptyDescription", "설명 준비 중") : Skill.Description);
	}
	if (Txt_Cooldown)
	{
		if (Skill.Cooldown > 0.f)
		{
			FNumberFormattingOptions Options;
			Options.MaximumFractionalDigits = 1;
			Txt_Cooldown->SetText(FText::Format(LOCTEXT("Cooldown", "재사용 대기시간 {0}초"), FText::AsNumber(Skill.Cooldown, &Options)));
			Txt_Cooldown->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Txt_Cooldown->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

#undef LOCTEXT_NAMESPACE