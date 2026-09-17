#pragma once
#include "Session.h"

class GameServer {
public:
    // Port defaults to Protocol.h's PORT constant; pass an explicit value
    // (e.g. from argv) so multiple instances can run on different ports.
    bool Init(unsigned short Port = PORT);

    // Spawns the worker thread pool and blocks (joins) forever, same as the
    // original single-threaded version just ran its loop forever.
    void Run();

private:
    void SendLoginFail(SOCKET client, const char* message);

    // Body of one worker thread: pulls completions off the IOCP and handles
    // them. Multiple threads run this concurrently; g_clients_mutex (Session.h)
    // serializes their access to `clients`.
    void WorkerLoop();

    SOCKET m_server = INVALID_SOCKET;
    SOCKET m_client_socket = INVALID_SOCKET;
    HANDLE m_iocp = nullptr;
    EXP_OVER m_accept_over{ IO_ACCEPT };
};
