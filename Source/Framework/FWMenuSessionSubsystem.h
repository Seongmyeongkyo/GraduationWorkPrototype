#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FWMenuSessionSubsystem.generated.h"

/** 로그인→로비 레벨 전환 중 표시 이름만 메모리에 유지. 실제 인증/비밀번호 저장 용도가 아닙니다. */
UCLASS()
class FRAMEWORK_API UFWMenuSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	void SetLocalDisplayName(const FString& Name);
	FString GetLocalDisplayName() const { return LocalDisplayName; }
	void ResetLocalDisplayName() { LocalDisplayName = TEXT("닉네임"); }

	/** 캐릭터 선택창 **/
	/** 확정된 캐릭터 ID 보관 (ID 검사는 호출하는 쪽에서 끝낸 값) **/
	void ConfirmCharacterSelection(FName CharacterId);
	/** 새 선택 세션 시작 시 이전 확정값 초기화 (닉네임은 건드리지 않음) **/
	void ResetCharacterSelection();
	FName GetSelectedCharacterId() const { return SelectedCharacterId; }
	bool IsCharacterSelectionConfirmed() const { return bCharacterSelectionConfirmed; }
private:
	UPROPERTY(Transient)
	FString LocalDisplayName = TEXT("닉네임");

	/** 캐릭터 선택창 관련 데이터 **/
	/** 위젯/Pawn 포인터가 아니라 ID만 저장 (GC 대상이 아니므로 UPROPERTY 불필요) **/
	FName SelectedCharacterId = NAME_None;
	bool bCharacterSelectionConfirmed = false;
};
