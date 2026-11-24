#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FWCharacterSelectTypes.generated.h"

class UTexture2D;

/** 캐릭터 ID 하나를 전달하는 이벤트 (버튼 클릭, 확정 요청) **/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWCharacterIdSignature, FName, CharacterId);

/** 캐릭터 선택 화면 진행 상태 (E_CharacterSelectPhase). 컨트롤러 C++ 안에서만 사용 **/
enum class EFWCharacterSelectPhase : uint8
{
	Initializing,	// 위젯 생성, 데이터 확인 중
	Selecting,		// 미리보기 변경 가능
	ConfirmPending,	// 확정 요청 후 결과 대기
	Confirmed,		// 확정 결과 저장 완료, 게임 진입 대기
	Travelling,		// Nature 맵으로 이동 중
	Failed			// 데이터/설정 오류로 진행 불가
};

/** 스킬 하나의 표시 데이터 (ST_CharacterSkillData) **/
USTRUCT(BlueprintType)
struct FFWCharacterSkillData
{
	GENERATED_BODY()

	/** Q, W, E, R. 비워 두면 슬롯 순서대로 Q/W/E/R 표시 **/
	UPROPERTY(EditAnywhere, Category = "Skill")
	FText Key;

	/** 기획에 따른 스킬 이름 **/
	UPROPERTY(EditAnywhere, Category = "Skill")
	FText DisplayName;

	/** 표시 아이콘 **/
	UPROPERTY(EditAnywhere, Category = "Skill")
	TObjectPtr<UTexture2D> Icon = nullptr;

	/** 스킬 설명 **/
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (MultiLine = true))
	FText Description;

	/** 재사용 대기시간(초). 0이면 툴팁에 표시하지 않음 **/
	UPROPERTY(EditAnywhere, Category = "Skill", meta = (ClampMin = "0.0"))
	float Cooldown = 0.f;

	/** 아이콘이나 이름 중 하나라도 있으면 제작된 스킬로 봄 (없으면 "준비 중" 표시) **/
	bool HasDisplayData() const { return Icon != nullptr || !DisplayName.IsEmpty(); }
};

/** 캐릭터 한 명의 선택창 표시 데이터 (ST_CharacterSelectData). 데이터 테이블 행 이름 = 캐릭터 ID **/
USTRUCT(BlueprintType)
struct FFWCharacterSelectData : public FTableRowBase
{
	GENERATED_BODY()

	/** 화면에 표시할 캐릭터 이름 **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	FText DisplayName;

	/** 목록 초상화 **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	TObjectPtr<UTexture2D> Portrait = nullptr;

	/** 중앙 일러스트 **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	TObjectPtr<UTexture2D> FullBody = nullptr;

	/** 역할 이름 **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	FText RoleName;

	/** 역할 배지 **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	TObjectPtr<UTexture2D> RoleBadge = nullptr;

	/** Q/W/E/R 순서의 스킬 표시 정보 (4개 미만이면 나머지는 "준비 중") **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	TArray<FFWCharacterSkillData> Skills;

	/** 중앙 일러스트 표시 크기 (1920x1080 기준). 0이면 원본 비율로 높이 1080에 맞춤 **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	FVector2D ArtSize = FVector2D::ZeroVector;

	/** 선택 허용 여부 **/
	UPROPERTY(EditAnywhere, Category = "CharacterSelect")
	bool bSelectable = true;
};

/** 팀원 슬롯 하나에 표시할 플레이어 상태 **/
USTRUCT(BlueprintType)
struct FFWTeamSelectPlayerView
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterSelect")
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterSelect")
	FString Nickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterSelect")
	FName CharacterId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterSelect")
	bool bIsLocalPlayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterSelect")
	bool bConfirmed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CharacterSelect")
	bool bOccupied = false;
};