// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWCharacterSelectTypes.h"
#include "FWCharacterSelectButton.generated.h"

class UButton;
class UImage;
class UTextBlock;

/** 캐릭터 목록의 초상화 버튼 하나 (WBP_CharacterSelectButton의 부모) **/
UCLASS(Abstract)
class FRAMEWORK_API UFWCharacterSelectButton : public UUserWidget
{
	GENERATED_BODY()

public:
	/** ID, 초상화, 이름 설정 **/
	void SetCharacterData(FName InCharacterId, const FFWCharacterSelectData& Data);

	/** 금색 강조 표시/숨김 **/
	void SetSelected(bool bIsSelected);

	/** 확정 요청 중, 확정 이후 클릭 제한 **/
	void SetSelectionLocked(bool bIsLocked);

	FName GetCharacterId() const { return CharacterId; }

	/** 부모 화면에 클릭한 캐릭터 ID 전달 (C++ AddDynamic으로만 바인딩) **/
	FFWCharacterIdSignature OnCharacterClicked;

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Select;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Portrait;

	/** 초상화 PNG에 이름이 이미 그려져 있으면 디자이너에서 빼도 됨 **/
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_CharacterName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_SelectedGold;

	/** 선택 불가 캐릭터, 또는 잠긴 상태에서 선택되지 않은 초상화의 색 **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect")
	FLinearColor DimmedTint = FLinearColor(0.45f, 0.45f, 0.45f, 1.f);

private:
	UFUNCTION()
	void HandleSelectClicked();

	/** 강조, 잠금, 어둡게 표시를 현재 상태에 맞춰 갱신 **/
	void RefreshVisualState();

	FName CharacterId = NAME_None;
	bool bSelected = false;
	bool bSelectionLocked = false;
	bool bSelectable = true;
};
