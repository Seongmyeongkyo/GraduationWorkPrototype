#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FWLoginGameMode.generated.h"

/** The title level has no playable pawn, gameplay camera or minimap. */
UCLASS()
class FRAMEWORK_API AFWLoginGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFWLoginGameMode();
};
