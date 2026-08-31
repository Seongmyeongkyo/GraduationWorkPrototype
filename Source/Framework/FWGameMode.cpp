// Copyright Epic Games, Inc. All Rights Reserved.

#include "FWGameMode.h"
#include "FWCharacter.h"
#include "FWPlayerController.h"

AFWGameMode::AFWGameMode()
{
	DefaultPawnClass = AFWCharacter::StaticClass();
	PlayerControllerClass = AFWPlayerController::StaticClass();
}
