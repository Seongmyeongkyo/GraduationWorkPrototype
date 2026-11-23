#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWCharacterSelectTypes.h"
#include "FWTeammateSelectSlot.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/** 하단 팀 상태 바의 한 칸: 닉네임, 초상화, 선택 중/확정 (WBP_TeammateSelectSlot의 부모) **/
UCLASS(Abstract)
class FRAMEWORK_API UFWTeammateSelectSlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 플레이어 상태 반영. View.bOccupied가 false면 빈 슬롯 **/
	void ApplyView(const FFWTeamSelectPlayerView& View, UTexture2D* PortraitTexture);

	/** 빈 슬롯 상태 **/
	void SetEmpty();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Portrait;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Nickname;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_Status;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_Status;

	/** T_Status_Selecting **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Status")
	TObjectPtr<UTexture2D> StatusSelectingTexture;

	/** T_Status_Ready **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Status")
	TObjectPtr<UTexture2D> StatusReadyTexture;

	/** 본인 닉네임 금색 #FFD76A **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Text")
	FLinearColor LocalNameColor = FLinearColor(FColor(0xFF, 0xD7, 0x6A));

	/** 다른 팀원 닉네임 아이보리색 **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Text")
	FLinearColor OtherNameColor = FLinearColor(FColor(0xF3, 0xE9, 0xD2));

	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Text")
	FLinearColor EmptyNameColor = FLinearColor(FColor(0x8C, 0x9A, 0x92));

	/** 선택 중 보라색 **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Text")
	FLinearColor SelectingStatusColor = FLinearColor(FColor(0xB3, 0x9C, 0xFF));

	/** 확정 초록색 **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Text")
	FLinearColor ReadyStatusColor = FLinearColor(FColor(0x5F, 0xD8, 0x93));

private:
	/** 둥근 모서리는 WBP의 RetainerBox(M_UI_TeammatePortraitRounded)가 처리하므로 텍스처만 지정 **/
	void SetPortrait(UTexture2D* Texture);
};