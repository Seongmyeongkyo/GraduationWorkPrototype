#include "FWLoginGameMode.h"
#include "FWLoginPlayerController.h"

AFWLoginGameMode::AFWLoginGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AFWLoginPlayerController::StaticClass();
}
