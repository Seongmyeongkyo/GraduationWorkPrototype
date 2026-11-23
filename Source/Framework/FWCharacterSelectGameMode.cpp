#include "FWCharacterSelectGameMode.h"
#include "FWSelectPlayerController.h"

AFWCharacterSelectGameMode::AFWCharacterSelectGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AFWSelectPlayerController::StaticClass();
}