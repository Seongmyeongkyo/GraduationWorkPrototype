#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/CharacterSelect/FWCharacterSelectTypes.h"
#include "FWSkillButtonWidget.generated.h"

class UImage;
class UTextBlock;
class UFWSkillTooltipWidget;

/** 스킬 아이콘 + 단축키 한 칸 (WBP_SkillButton의 부모) **/
UCLASS(Abstract)
class FRAMEWORK_API UFWSkillButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 아이콘, 단축키, 툴팁 내용을 함께 갱신. bHasData가 false면 "준비 중" 표시 **/
	void SetSkillData(const FFWCharacterSkillData& Skill, const FText& FallbackKey, bool bHasData);

protected:
	virtual void NativeOnInitialized() override;

	/** 툴팁이 이 이미지에 붙으므로 Visibility는 Visible이어야 함 (코드에서도 지정) **/
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Icon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Key;

	/** WBP_SkillTooltip **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect")
	TSubclassOf<UFWSkillTooltipWidget> TooltipWidgetClass;

	/** 미제작 스킬 칸의 빈 아이콘 색 **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect")
	FLinearColor PlaceholderTint = FLinearColor(1.f, 1.f, 1.f, 0.15f);

private:
	/** 툴팁 위젯은 NativeOnInitialized에서 한 번만 만들고 내용만 바꿈 **/
	UPROPERTY(Transient)
	TObjectPtr<UFWSkillTooltipWidget> SkillTooltip;
};