#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FWLobbyGameMode.generated.h"

/** L_Lobby 전용. 로비 컨트롤러만 생성하고 플레이 캐릭터/HUD는 생성하지 않습니다. */
UCLASS()
class FRAMEWORK_API AFWLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	AFWLobbyGameMode();
};
