#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Frontend/CharacterSelect/FWCharacterSelectTypes.h"
#include "FWCharacterSelectWidget.generated.h"

class UButton;
class UDataTable;
class UImage;
class UTextBlock;
class UTexture2D;
class UUniformGridPanel;
class UFWCharacterSelectButton;
class UFWSkillButtonWidget;
class UFWTeammateSelectSlot;

/**
 * 캐릭터 선택 화면 전체 (WBP_CharacterSelect의 부모).
 * 표시와 입력 전달만 담당하고, 상태 전환/맵 이동은 AFWSelectPlayerController가 담당한다.
 */
UCLASS(Abstract)
class FRAMEWORK_API UFWCharacterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 목록과 본인 정보 초기화 **/
	void InitializeScreen(const FString& Nickname);

	/** 사용자 입력용 미리보기. 잠금/대기 중이거나 선택 불가 캐릭터면 false **/
	bool PreviewCharacter(FName CharacterId);

	/** 잠금 여부와 상관없이 그림/이름/역할/스킬/강조/본인 슬롯을 갱신 (최종 결과 반영용) **/
	bool ApplyCharacterDisplay(FName CharacterId);

	/** 현재 미리보기 ID **/
	FName GetCurrentCharacterId() const { return SelectedCharacterId; }

	/** 남은 시간 00:30 형식 표시 **/
	void SetRemainingSeconds(int32 Seconds);

	/** 시간 정보를 아직 받지 못했을 때 --:-- 표시 **/
	void ClearRemainingTime();

	/** 요청 대기 표시와 입력 잠금/복구 **/
	void SetRequestPending(bool bIsPending);

	/** 버튼 잠금 및 본인 확정 표시 **/
	void SetConfirmedState();

	/** 진행할 수 없는 오류: 입력을 잠그고 안내 **/
	void SetFailedState(const FText& Message);

	void ShowSelectionError(const FText& Message);

	void ClearStatusMessage();

	/** 팀원 슬롯 갱신. 본인은 항상 Slot_Local, 다른 팀원은 PlayerId 오름차순으로 Slot_TeamA/B **/
	void ApplyTeamSlots(const TArray<FFWTeamSelectPlayerView>& PlayerViews);

	/** 0 = 투명, 1 = 검정 (맵 이동 페이드) **/
	void SetTransitionOpacity(float Opacity);

	/** 데이터 행이 있고 bSelectable=True인지 **/
	bool IsSelectableCharacter(FName CharacterId) const;

	/** RosterOrder에서 처음 선택 가능한 캐릭터 (없으면 None) **/
	FName FindFirstSelectableCharacterId() const;

	/** 데이터 테이블 행 조회 (없으면 nullptr) **/
	const FFWCharacterSelectData* FindCharacterData(FName CharacterId) const;

	/** 컨트롤러에 확정 요청 전달 (C++ AddDynamic으로만 바인딩) **/
	FFWCharacterIdSignature OnConfirmRequested;

protected:
	virtual void NativeOnInitialized() override;

	/** DT_CharacterSelect (Row Structure: FWCharacterSelectData) **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Data")
	TObjectPtr<UDataTable> CharacterDataTable;

	/** 목록 배치 순서. 행 조회 순서에 의존하지 않음 **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Data")
	TArray<FName> RosterOrder = { FName(TEXT("Luna")), FName(TEXT("Robin")), FName(TEXT("Miro")), FName(TEXT("Kain")), FName(TEXT("Toto")), FName(TEXT("Noah")) };

	/** WBP_CharacterSelectButton **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Roster")
	TSubclassOf<UFWCharacterSelectButton> CharacterButtonClass;

	/** 목록 열 수 (Row = Index / Columns, Column = Index % Columns) **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Roster", meta = (ClampMin = "1"))
	int32 RosterColumns = 2;

	/** 중앙 일러스트의 가로 중심 (1920x1080 디자인 좌표) **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Art")
	float ArtCenterX = 890.f;

	/** 중앙 일러스트의 아래쪽 기준선 **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Art")
	float ArtBottomY = 1080.f;

	/** ArtSize가 0인 행에서 사용할 높이 (가로는 원본 비율로 계산) **/
	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Art")
	float ArtDefaultHeight = 1080.f;

	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Message")
	FLinearColor StatusMessageColor = FLinearColor(FColor(0xF3, 0xE9, 0xD2));

	UPROPERTY(EditDefaultsOnly, Category = "CharacterSelect|Message")
	FLinearColor ErrorMessageColor = FLinearColor(FColor(0xFF, 0x8A, 0x7A));

	// ---- 디자이너 위젯 (이름이 같아야 연결됨) ----

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Img_CharacterArt;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_CharacterName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_RoleName;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_RoleBadge;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFWSkillButtonWidget> SkillButton_Q;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFWSkillButtonWidget> SkillButton_W;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFWSkillButtonWidget> SkillButton_E;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFWSkillButtonWidget> SkillButton_R;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> UniformGrid_Characters;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_RemainingTime;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Confirm;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Txt_ConfirmLabel;

	/** 다른 팀원 슬롯 (왼쪽부터) **/
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFWTeammateSelectSlot> Slot_TeamA;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFWTeammateSelectSlot> Slot_TeamB;

	/** 본인 슬롯 (오른쪽 끝) **/
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UFWTeammateSelectSlot> Slot_Local;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Txt_StatusMessage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Img_FadeBlack;

private:
	UFUNCTION()
	void HandleCharacterButtonClicked(FName CharacterId);

	UFUNCTION()
	void HandleConfirmClicked();

	/** RosterOrder 순서로 선택 버튼 생성 (InitializeScreen에서 한 번) **/
	void BuildCharacterButtons();
	void ShowStatusMessage(const FText& Message);
	void ApplyCharacterArt(const FFWCharacterSelectData& Data);
	void ApplySkills(const FFWCharacterSelectData& Data);
	void RefreshButtonSelection();
	void RefreshInputLock();
	void RefreshLocalSlot();
	/** 팀원 슬롯에 표시할 초상화 (목록 초상화와 같은 텍스처) **/
	UTexture2D* FindPortrait(FName CharacterId) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UFWCharacterSelectButton>> CharacterButtonRefs;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UFWSkillButtonWidget>> SkillButtons;

	/** 본인 슬롯 표시 상태 (닉네임, 확정 여부) **/
	FFWTeamSelectPlayerView LocalView;

	FName SelectedCharacterId = NAME_None;

	/** 확정 이후(또는 오류) 입력 잠금 **/
	bool bSelectionLocked = false;

	/** 확정 요청 결과 대기 중 입력 잠금 **/
	bool bRequestPending = false;
};