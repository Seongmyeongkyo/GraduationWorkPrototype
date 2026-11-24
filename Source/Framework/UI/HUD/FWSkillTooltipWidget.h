// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/CharacterSelect/FWCharacterSelectTypes.h"
#include "FWSkillTooltipWidget.generated.h"

class UTextBlock;

/** 스킬 아이콘에 마우스를 올렸을 때 뜨는 설명 (WBP_SkillTooltip의 부모) **/
UCLASS(Abstract)
class FRAMEWORK_API UFWSkillTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** bHasData가 false면 "준비 중" 문구 표시 **/
	void SetSkillData(const FFWCharacterSkillData& Skill, const FText& KeyText, bool bHasData);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_SkillName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Description;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_Cooldown;
};
