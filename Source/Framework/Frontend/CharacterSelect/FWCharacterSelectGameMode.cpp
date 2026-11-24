#include "Frontend/CharacterSelect/FWCharacterSelectGameMode.h"
#include "Frontend/CharacterSelect/FWSelectPlayerController.h"

AFWCharacterSelectGameMode::AFWCharacterSelectGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AFWSelectPlayerController::StaticClass();
}