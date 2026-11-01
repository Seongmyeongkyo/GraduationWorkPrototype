#include "FWLobbyGameMode.h"
#include "FWLobbyPlayerController.h"

AFWLobbyGameMode::AFWLobbyGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AFWLobbyPlayerController::StaticClass();
}
