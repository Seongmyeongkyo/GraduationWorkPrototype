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
private:
	UPROPERTY(Transient)
	FString LocalDisplayName = TEXT("닉네임");
};
