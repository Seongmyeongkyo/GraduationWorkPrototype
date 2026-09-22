#include <cstdlib>
#include "GameServer.h"

// Usage: Server.exe [port]
// Port defaults to Protocol.h's PORT constant when omitted. Passing a port
// explicitly is what lets multiple instances run side by side later
// (e.g. one per match), without any other code changes.
int main(int argc, char* argv[]) {
    unsigned short port = PORT;
    if (argc >= 2) {
        port = static_cast<unsigned short>(std::atoi(argv[1]));
    }

    GameServer server;
    if (!server.Init(port)) return 1;
    server.Run();
    return 0;
}
