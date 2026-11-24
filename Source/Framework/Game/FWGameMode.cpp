// Copyright Epic Games, Inc. All Rights Reserved.

#include "Game/FWGameMode.h"
#include "Character/FWCharacter.h"
#include "Game/FWPlayerController.h"

AFWGameMode::AFWGameMode()
{
	DefaultPawnClass = AFWCharacter::StaticClass();
	PlayerControllerClass = AFWPlayerController::StaticClass();
}
