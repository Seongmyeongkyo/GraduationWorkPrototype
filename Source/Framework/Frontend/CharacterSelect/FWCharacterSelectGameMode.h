#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FWCharacterSelectGameMode.generated.h"

/** L_CharacterSelect 전용 (BP_GM_CharacterSelect의 부모). 선택 컨트롤러만 생성하고 플레이 캐릭터/HUD는 생성하지 않습니다. */
UCLASS()
class FRAMEWORK_API AFWCharacterSelectGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AFWCharacterSelectGameMode();
};