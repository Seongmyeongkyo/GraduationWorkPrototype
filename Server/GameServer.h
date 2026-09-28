#pragma once
#include "Session.h"

class GameServer {
public:
    // 기본은 Protocol.h의 PORT. 인스턴스마다 다른 포트로 여러 개 띄울 수 있게 인자로 받는다.
    bool Init(unsigned short Port = PORT);

    // 워커 스레드를 띄우고 영원히 대기한다.
    void Run();

private:
    void SendLoginFail(SOCKET client, const char* message);

    // 다음 AcceptEx를 건다. AcceptEx는 항상 하나만 걸려 있으므로 완료(성공/실패)마다
    // 반드시 다시 걸어야 한다. 한 번이라도 빠뜨리면 이후 접속을 받지 못한다.
    void PostAccept();

    // 세션을 끊고 다른 플레이어에게 제거를 알린 뒤 초기화한다.
    // 범위 밖이거나 이미 끊긴 id면 아무것도 안 함. g_clients_mutex를 잡은 상태에서 호출할 것.
    void Disconnect(int p_id);

    // 워커 스레드 본체. 여러 스레드가 동시에 돌며 g_clients_mutex로 clients 접근을 직렬화한다.
    void WorkerLoop();

    SOCKET m_server = INVALID_SOCKET;
    SOCKET m_client_socket = INVALID_SOCKET;
    HANDLE m_iocp = nullptr;
    EXP_OVER m_accept_over{ IO_ACCEPT };
};
