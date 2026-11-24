#include "Frontend/Lobby/FWLobbyGameMode.h"
#include "Frontend/Lobby/FWLobbyPlayerController.h"

AFWLobbyGameMode::AFWLobbyGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	PlayerControllerClass = AFWLobbyPlayerController::StaticClass();
}
