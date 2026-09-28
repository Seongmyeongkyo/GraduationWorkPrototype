#pragma once
#include <string>

// 비동기 로거: Log()는 시각을 찍어 큐에 넣기만 하고, 로그 전용 스레드가 콘솔과 파일에 쓴다.
// 호출하는 쪽(락을 잡은 워커 등)이 입출력을 기다리지 않는다. 여러 스레드에서 호출해도 안전.
namespace Logger {
    // Logs/ 폴더를 만들고 Logs/server_<port>.txt를 추가 모드로 연 뒤 로그 스레드를 시작한다.
    void Init(unsigned short port);

    void Log(const std::string& message);

    // 큐에 남은 로그를 모두 쓰고 로그 스레드를 종료한다. 여러 번, 여러 스레드에서 호출해도 안전.
    void Shutdown();
}
