#pragma once
#include "Session.h"

class GameServer {
public:
    // Port defaults to Protocol.h's PORT constant; pass an explicit value
    // (e.g. from argv) so multiple instances can run on different ports.
    bool Init(unsigned short Port = PORT);
    void Run();

private:
    void SendLoginFail(SOCKET client, const char* message);

    SOCKET m_server = INVALID_SOCKET;
    SOCKET m_client_socket = INVALID_SOCKET;
    HANDLE m_iocp = nullptr;
    EXP_OVER m_accept_over{ IO_ACCEPT };
};
