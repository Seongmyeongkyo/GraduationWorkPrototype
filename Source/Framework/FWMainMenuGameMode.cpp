#include "FWMainMenuGameMode.h"
#include "FWMainMenuPlayerController.h"

AFWMainMenuGameMode::AFWMainMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AFWMainMenuPlayerController::StaticClass();
}
