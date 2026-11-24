#include "Frontend/Login/FWLoginGameMode.h"
#include "Frontend/Login/FWLoginPlayerController.h"

AFWLoginGameMode::AFWLoginGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AFWLoginPlayerController::StaticClass();
}
