#include <cstdlib>
#include "GameServer.h"
#include "Logger.h"

namespace {
    // Ctrl+C나 콘솔 창 닫기는 프로세스를 바로 종료시키므로, 그 전에 큐에 남은 로그를 파일에 쓴다.
    BOOL WINAPI OnConsoleClose(DWORD /*ctrl_type*/) {
        Logger::Shutdown();
        return FALSE; // 기본 처리기로 넘겨서 프로세스 종료
    }
}

// 사용법: Server.exe [포트]
// 포트를 생략하면 Protocol.h의 PORT. 인스턴스(매치)마다 다른 포트로 여러 개 띄울 때 사용한다.
int main(int argc, char* argv[]) {
    unsigned short port = PORT;
    if (argc >= 2) {
        port = static_cast<unsigned short>(std::atoi(argv[1]));
    }

    SetConsoleCtrlHandler(OnConsoleClose, TRUE);

    GameServer server;
    if (!server.Init(port)) {
        Logger::Shutdown();
        return 1;
    }
    server.Run();
    Logger::Shutdown();
    return 0;
}
