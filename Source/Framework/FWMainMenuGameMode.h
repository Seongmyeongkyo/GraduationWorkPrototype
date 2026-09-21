#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FWMainMenuGameMode.generated.h"

/** The title level has no playable pawn, gameplay camera or minimap. */
UCLASS()
class FRAMEWORK_API AFWMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFWMainMenuGameMode();
};
