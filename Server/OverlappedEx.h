#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>

#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "WS2_32.lib")

constexpr int BUF_SIZE = 200;

enum IOType { IO_SEND, IO_RECV, IO_ACCEPT };

class EXP_OVER {
public:
    WSAOVERLAPPED m_over;
    IOType  m_iotype;
    WSABUF  m_wsa;
    char  m_buff[BUF_SIZE];
    EXP_OVER() {
        ZeroMemory(&m_over, sizeof(m_over));
        m_wsa.buf = m_buff;
        m_wsa.len = BUF_SIZE;
    }
    EXP_OVER(IOType iot) : m_iotype(iot) {
        ZeroMemory(&m_over, sizeof(m_over));
        m_wsa.buf = m_buff;
        m_wsa.len = BUF_SIZE;
    }
};
